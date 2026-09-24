// ESPtember Day 21 — Yo (Waveshare half)
// The other end of the pager: ESP-IDF on the AMOLED 1.8, receiving the
// exact same 24-byte frames the Arduino StopWatch half sends. Nothing
// above the packet is shared — different vendor, framework, UI toolkit —
// and none of it matters to the radio. Board bring-up (pmu_init,
// panel_reset_release) carries over from day 01.
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_mac.h"
#include "esp_timer.h"

// --- The wire protocol (shared with the Arduino half, byte for byte) ----
#define YO_MAGIC 0x30594F45u
#define YO_CHANNEL 1

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t type; // 0 = hello, 1 = poke
    char name[12];
} yo_msg_t;

static const char *MY_NAME = "AMOLED-1.8";

// --- AXP2101 power management (day 01, unchanged) ----------------------
#define AXP2101_ADDR 0x34

static i2c_master_dev_handle_t pmu;

static uint8_t pmu_read(uint8_t reg)
{
    uint8_t val = 0;
    i2c_master_transmit_receive(pmu, &reg, 1, &val, 1, 100);
    return val;
}

static void pmu_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    i2c_master_transmit(pmu, buf, 2, 100);
}

static void pmu_init(void)
{
    bsp_i2c_init();
    i2c_device_config_t cfg = {
        .device_address = AXP2101_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(bsp_i2c_get_handle(), &cfg, &pmu));
    pmu_write(0x16, (pmu_read(0x16) & 0xF8) | 0x02);
    pmu_write(0x62, (pmu_read(0x62) & 0xE0) | 0x09);
}

// --- V2 panel reset (day 01, unchanged) --------------------------------
#include "esp_io_expander_tca9554.h"

static void panel_reset_release(void)
{
    esp_io_expander_handle_t expander = NULL;
    ESP_ERROR_CHECK(esp_io_expander_new_i2c_tca9554(
        bsp_i2c_get_handle(), ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000,
        &expander));
    const uint32_t reset_pins =
        IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1 | IO_EXPANDER_PIN_NUM_2;
    ESP_ERROR_CHECK(
        esp_io_expander_set_dir(expander, reset_pins, IO_EXPANDER_OUTPUT));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander, reset_pins, 1));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander, reset_pins, 0));
    vTaskDelay(pdMS_TO_TICKS(300));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander, reset_pins, 1));
}

// --- Roster --------------------------------------------------------------
typedef struct {
    uint8_t mac[6];
    char name[12];
    int64_t last_seen_us;
    lv_obj_t *button; // its row in the list
} peer_t;

#define MAX_PEERS 8
static peer_t peers[MAX_PEERS];
static int peer_count = 0;

static peer_t *find_peer(const uint8_t *mac)
{
    for (int i = 0; i < peer_count; i++)
        if (!memcmp(peers[i].mac, mac, 6)) return &peers[i];
    return NULL;
}

// --- UI --------------------------------------------------------------------
#define ESPTEMBER_ORANGE 0xff5b04

static lv_obj_t *peer_list;
static lv_obj_t *empty_label;
static lv_obj_t *poke_overlay;
static lv_obj_t *poke_label;
static lv_timer_t *poke_hide_timer;

static void send_poke(int index);

static void row_cb(lv_event_t *e)
{
    send_poke((int)(intptr_t)lv_event_get_user_data(e));
}

static void poke_hide_cb(lv_timer_t *t)
{
    (void)t;
    lv_obj_add_flag(poke_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_timer_pause(poke_hide_timer);
}

static void build_ui(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_text_font(screen, &lv_font_montserrat_28, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "YO");
    lv_obj_set_style_text_color(title, lv_color_hex(ESPTEMBER_ORANGE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    empty_label = lv_label_create(screen);
    lv_label_set_text(empty_label, "listening...");
    lv_obj_set_style_text_color(empty_label, lv_color_hex(0x888888), 0);
    lv_obj_center(empty_label);

    peer_list = lv_list_create(screen);
    lv_obj_set_size(peer_list, lv_pct(94), 300);
    lv_obj_align(peer_list, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_bg_color(peer_list, lv_color_black(), 0);
    lv_obj_set_style_border_width(peer_list, 0, 0);

    lv_obj_t *hint = lv_label_create(screen);
    lv_label_set_text(hint, "tap a board to YO it");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -18);

    // Full-screen receipt overlay, hidden until poked.
    poke_overlay = lv_obj_create(screen);
    lv_obj_set_size(poke_overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(poke_overlay, lv_color_hex(ESPTEMBER_ORANGE), 0);
    lv_obj_add_flag(poke_overlay, LV_OBJ_FLAG_HIDDEN);
    poke_label = lv_label_create(poke_overlay);
    lv_obj_set_style_text_color(poke_label, lv_color_black(), 0);
    lv_obj_center(poke_label);
    poke_hide_timer = lv_timer_create(poke_hide_cb, 2000, NULL);
    lv_timer_pause(poke_hide_timer);
}

static void roster_add_row(peer_t *p)
{
    lv_obj_add_flag(empty_label, LV_OBJ_FLAG_HIDDEN);
    p->button = lv_list_add_button(peer_list, LV_SYMBOL_WIFI, p->name);
    lv_obj_set_style_bg_color(p->button, lv_color_black(), 0);
    lv_obj_set_style_text_color(p->button, lv_color_white(), 0);
    lv_obj_set_style_pad_ver(p->button, 20, 0);
    lv_obj_add_event_cb(p->button, row_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)(p - peers));
}

// --- ESP-NOW -----------------------------------------------------------------
static const uint8_t BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static void on_receive(const esp_now_recv_info_t *info, const uint8_t *data,
                       int len)
{
    if (len != sizeof(yo_msg_t)) return;
    yo_msg_t msg;
    memcpy(&msg, data, sizeof(msg));
    if (msg.magic != YO_MAGIC) return;
    msg.name[sizeof(msg.name) - 1] = 0;

    peer_t *p = find_peer(info->src_addr);
    if (!p && peer_count < MAX_PEERS) {
        p = &peers[peer_count++];
        memcpy(p->mac, info->src_addr, 6);
        strncpy(p->name, msg.name, sizeof(p->name));
        esp_now_peer_info_t reg = {.channel = YO_CHANNEL, .ifidx = WIFI_IF_STA};
        memcpy(reg.peer_addr, info->src_addr, 6);
        esp_now_add_peer(&reg);
        printf("D20_PEER " MACSTR " %s\n", MAC2STR(info->src_addr), msg.name);
        bsp_display_lock(0);
        roster_add_row(p);
        bsp_display_unlock();
    }
    if (!p) return;
    p->last_seen_us = esp_timer_get_time();

    if (msg.type == 1) {
        printf("D20_POKED by=%s\n", msg.name);
        bsp_display_lock(0);
        lv_label_set_text_fmt(poke_label, "YO! from %s", msg.name);
        lv_obj_remove_flag(poke_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_timer_reset(poke_hide_timer);
        lv_timer_resume(poke_hide_timer);
        bsp_display_unlock();
    }
}

static void on_sent(const wifi_tx_info_t *info, esp_now_send_status_t status)
{
    (void)info;
    printf("D20_SENT %s\n",
           status == ESP_NOW_SEND_SUCCESS ? "acked" : "lost");
}

static void send_hello(void)
{
    yo_msg_t msg = {YO_MAGIC, 0, {0}};
    strncpy(msg.name, MY_NAME, sizeof(msg.name));
    esp_now_send(BROADCAST, (const uint8_t *)&msg, sizeof(msg));
}

static void send_poke(int index)
{
    if (index >= peer_count) return;
    yo_msg_t msg = {YO_MAGIC, 1, {0}};
    strncpy(msg.name, MY_NAME, sizeof(msg.name));
    esp_now_send(peers[index].mac, (const uint8_t *)&msg, sizeof(msg));
    printf("D20_POKE to=%s\n", peers[index].name);
}

static void espnow_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(YO_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_receive));
    ESP_ERROR_CHECK(esp_now_register_send_cb(on_sent));
    esp_now_peer_info_t bc = {.channel = YO_CHANNEL, .ifidx = WIFI_IF_STA};
    memcpy(bc.peer_addr, BROADCAST, 6);
    ESP_ERROR_CHECK(esp_now_add_peer(&bc));
}

// Serial test hook: 'p' pokes peer 0, 's' prints the roster.
static void console_task(void *arg)
{
    (void)arg;
    while (1) {
        const int c = getchar();
        if (c == 'p') send_poke(0);
        else if (c == 's')
            printf("D20_STATUS peers=%d\n", peer_count);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void)
{
    pmu_init();
    panel_reset_release();
    bsp_display_start();
    bsp_display_backlight_on();

    bsp_display_lock(0);
    build_ui();
    bsp_display_unlock();

    espnow_init();

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    printf("D20_READY name=%s mac=" MACSTR "\n", MY_NAME, MAC2STR(mac));

    xTaskCreate(console_task, "console", 4096, NULL, 5, NULL);
    while (1) {
        send_hello();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
