// ESPtember Day 25 — Internet Walkie-Talkie, receiver half (Waveshare 1.8)
// Day 24's receiver with the transport swapped: voice frames arrive as
// binary WebSocket messages from a Cloudflare Durable Object instead of
// ESP-NOW broadcasts. The jitter buffer, the codec path, the channel
// squelch — untouched. Provisioning is day 22's captive portal, lifted
// whole: no saved Wi-Fi means the board becomes esptember-setup first.
// Board bring-up (pmu_init, panel_reset_release) carries over from
// day 01.
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c_master.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_websocket_client.h"
#include "esp_crt_bundle.h"
#include "esp_codec_dev.h"
#include "esp_timer.h"
#include "portal.h"

#define RELAY_URI_FMT "wss://esptember-walkie-relay.chantastic.workers.dev/channel/%d"
#define WT_MAGIC 0x30574B54u
#define SAMPLE_RATE 8000
#define FRAME_SAMPLES 120

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t seq;
    uint8_t channel;
    uint8_t flags;
    int16_t pcm[FRAME_SAMPLES];
} wt_frame_t;

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

// --- Jitter buffer (day 24, unchanged) ------------------------------------
#define QUEUE_FRAMES 16
#define PREBUFFER 4

typedef struct {
    int16_t pcm[FRAME_SAMPLES];
} pcm_frame_t;

static QueueHandle_t audio_queue;
static volatile uint8_t my_channel = 1;
static volatile uint32_t frames_rx = 0, frames_dropped = 0;
static volatile uint16_t last_seq = 0;
static volatile uint32_t frames_lost = 0;
static volatile int64_t last_rx_us = 0;
static volatile bool ws_up = false;

static void handle_frame(const uint8_t *data, int len)
{
    if (len != sizeof(wt_frame_t)) return;
    const wt_frame_t *frame = (const wt_frame_t *)data;
    if (frame->magic != WT_MAGIC) return;
    // The relay is the squelch now — the room *is* the channel — but
    // the byte still gets checked: defense against a misjoined room.
    if (frame->channel != my_channel) return;
    if (!(frame->flags & 1) && frame->seq != (uint16_t)(last_seq + 1))
        frames_lost++;
    last_seq = frame->seq;
    frames_rx++;
    last_rx_us = esp_timer_get_time();
    pcm_frame_t out;
    memcpy(out.pcm, frame->pcm, sizeof(out.pcm));
    if (xQueueSend(audio_queue, &out, 0) != pdTRUE) frames_dropped++;
}

static void audio_task(void *arg)
{
    esp_codec_dev_handle_t spk = (esp_codec_dev_handle_t)arg;
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 2 * SAMPLE_RATE, // 2x upsample, as day 24
        .channel = 1,
        .bits_per_sample = 16,
    };
    printf("D24_CODEC open=%d\n", esp_codec_dev_open(spk, &fs));
    esp_codec_dev_set_out_vol(spk, 85);
    static int16_t out[2 * FRAME_SAMPLES];
    pcm_frame_t frame;
    bool flowing = false;
    while (1) {
        const int waiting = uxQueueMessagesWaiting(audio_queue);
        if (!flowing && waiting >= PREBUFFER) flowing = true;
        if (flowing && waiting == 0) flowing = false;
        if (flowing && xQueueReceive(audio_queue, &frame, pdMS_TO_TICKS(30))) {
            for (int i = 0; i < FRAME_SAMPLES; i++) {
                out[2 * i] = frame.pcm[i];
                out[2 * i + 1] = frame.pcm[i];
            }
            if (esp_codec_dev_write(spk, out, sizeof(out)) != 0)
                vTaskDelay(2);
        } else {
            vTaskDelay(1); // >= 1 tick, per day 24's watchdog lesson
        }
    }
}

// --- WebSocket -----------------------------------------------------------------
static esp_websocket_client_handle_t ws;

static void ws_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base;
    const esp_websocket_event_data_t *ev = data;
    if (id == WEBSOCKET_EVENT_CONNECTED) {
        ws_up = true;
        printf("D24_WS connected\n");
    } else if (id == WEBSOCKET_EVENT_DISCONNECTED) {
        ws_up = false;
    } else if (id == WEBSOCKET_EVENT_DATA && ev->op_code == 2) {
        // Binary frames only; a DO delivers messages whole.
        handle_frame((const uint8_t *)ev->data_ptr, ev->data_len);
    }
}

static void ws_join_channel(int channel)
{
    char uri[96];
    snprintf(uri, sizeof(uri), RELAY_URI_FMT, channel);
    if (ws) {
        esp_websocket_client_stop(ws);
        esp_websocket_client_destroy(ws);
    }
    const esp_websocket_client_config_t cfg = {
        .uri = uri,
        .reconnect_timeout_ms = 2000,
        .network_timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    ws = esp_websocket_client_init(&cfg);
    esp_websocket_register_events(ws, WEBSOCKET_EVENT_ANY, ws_event, NULL);
    esp_websocket_client_start(ws);
    printf("D24_JOIN %d\n", channel);
}

// --- UI (day 24's, plus link state) --------------------------------------------
#define ESPTEMBER_ORANGE 0xff5b04

static lv_obj_t *channel_label;
static lv_obj_t *state_label;

static void channel_cb(lv_event_t *e)
{
    const int delta = (int)(intptr_t)lv_event_get_user_data(e);
    my_channel = (my_channel + 21 + delta) % 22 + 1;
    lv_label_set_text_fmt(channel_label, "%d", my_channel);
    ws_join_channel(my_channel);
}

static lv_obj_t *make_channel_button(lv_obj_t *parent, const char *text,
                                     int delta)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 100, 80);
    lv_obj_add_event_cb(btn, channel_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)delta);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return btn;
}

static void build_ui(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_text_font(screen, &lv_font_montserrat_28, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "WALKIE  NET RX");
    lv_obj_set_style_text_color(title, lv_color_hex(ESPTEMBER_ORANGE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    channel_label = lv_label_create(screen);
    lv_obj_set_style_text_font(channel_label, &lv_font_montserrat_48, 0);
    lv_label_set_text_fmt(channel_label, "%d", my_channel);
    lv_obj_align(channel_label, LV_ALIGN_CENTER, 0, -60);

    lv_obj_t *row = lv_obj_create(screen);
    lv_obj_set_size(row, lv_pct(90), 100);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    make_channel_button(row, LV_SYMBOL_MINUS, -1);
    make_channel_button(row, LV_SYMBOL_PLUS, 1);

    state_label = lv_label_create(screen);
    lv_obj_set_style_text_color(state_label, lv_color_hex(0x888888), 0);
    lv_obj_align(state_label, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_label_set_text(state_label, "offline");
}

static void ui_tick(lv_timer_t *t)
{
    (void)t;
    const bool live = esp_timer_get_time() - last_rx_us < 300000;
    const char *text = live    ? "RECEIVING"
                       : ws_up ? "relay linked"
                               : "offline";
    lv_label_set_text(state_label, text);
    lv_obj_set_style_text_color(state_label,
                                live ? lv_color_hex(0x00ff00)
                                     : lv_color_hex(0x888888),
                                0);
}

// --- Wi-Fi station -----------------------------------------------------------
static volatile bool got_ip = false;

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id,
                          void *data)
{
    (void)arg; (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) esp_wifi_connect();
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        got_ip = false;
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        got_ip = true;
        printf("D24_CONNECTED\n");
    }
}

static void station_start(const char *ssid, const char *pass)
{
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi_event,
                               NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_wifi_event,
                               NULL);
    wifi_config_t sta = {0};
    strncpy((char *)sta.sta.ssid, ssid, sizeof(sta.sta.ssid));
    strncpy((char *)sta.sta.password, pass, sizeof(sta.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void on_portal_submit(const char *ssid, const char *pass)
{
    portal_save_creds(ssid, pass);
    vTaskDelay(pdMS_TO_TICKS(1500));
    esp_restart();
}

static void console_task(void *arg)
{
    (void)arg;
    while (1) {
        const int c = getchar();
        if (c == 's')
            printf("D24_STATUS ch=%d wifi=%d ws=%d rx=%lu lost=%lu dropped=%lu\n",
                   my_channel, (int)got_ip, (int)ws_up,
                   (unsigned long)frames_rx, (unsigned long)frames_lost,
                   (unsigned long)frames_dropped);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void)
{
    pmu_init();
    panel_reset_release();

    bsp_audio_init(NULL);
    esp_codec_dev_handle_t spk = bsp_audio_codec_speaker_init();
    audio_queue = xQueueCreate(QUEUE_FRAMES, sizeof(pcm_frame_t));
    xTaskCreate(audio_task, "audio", 4096, spk, 7, NULL);

    bsp_display_start();
    bsp_display_backlight_on();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    char ssid[33] = {0}, pass[65] = {0};
    if (!portal_load_creds(ssid, sizeof(ssid), pass, sizeof(pass))) {
        // No Wi-Fi yet: become the setup page (day 22, verbatim).
        bsp_display_lock(0);
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
        lv_obj_set_style_text_font(lv_screen_active(), &lv_font_montserrat_28,
                                   0);
        lv_obj_t *label = lv_label_create(lv_screen_active());
        lv_label_set_text(label,
                          "SETUP\n\nJoin Wi-Fi network\n\"esptember-setup\"");
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(label);
        bsp_display_unlock();
        printf("D24_MODE portal\n");
        portal_start(on_portal_submit);
        return;
    }

    bsp_display_lock(0);
    build_ui();
    lv_timer_create(ui_tick, 200, NULL);
    bsp_display_unlock();

    station_start(ssid, pass);
    while (!got_ip) vTaskDelay(pdMS_TO_TICKS(200));
    ws_join_channel(my_channel);

    xTaskCreate(console_task, "console", 4096, NULL, 5, NULL);
    printf("D24_READY rx\n");
}
