// ESPtember Day 21 — Captive Portal
// The gateway lesson: every internet-facing day after this one assumes
// the board can get online without credentials in the repo. No saved
// Wi-Fi -> the board becomes an open access point named
// "esptember-setup"; joining it pops a setup page (via a DNS server
// that answers every question with our address); the form saves SSID +
// password to NVS and reboots into station mode. Hold BOOT during
// power-up to erase and start over. Board bring-up (pmu_init,
// panel_reset_release) carries over from day 01.
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "portal.h"

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

// --- UI --------------------------------------------------------------------
#define ESPTEMBER_ORANGE 0xff5b04

static lv_obj_t *title_label;
static lv_obj_t *body_label;

static void ui_show(const char *title, const char *body)
{
    bsp_display_lock(0);
    lv_label_set_text(title_label, title);
    lv_label_set_text(body_label, body);
    bsp_display_unlock();
}

static void build_ui(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_text_font(screen, &lv_font_montserrat_28, 0);

    title_label = lv_label_create(screen);
    lv_obj_set_style_text_color(title_label, lv_color_hex(ESPTEMBER_ORANGE),
                                0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 40);
    lv_label_set_text(title_label, "");

    body_label = lv_label_create(screen);
    lv_obj_set_width(body_label, lv_pct(88));
    lv_obj_set_style_text_align(body_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(body_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(body_label, "");
}

// --- Station mode -------------------------------------------------------------
static volatile bool got_ip = false;
static char ip_text[20] = "";

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id,
                          void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) esp_wifi_connect();
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        got_ip = false;
        esp_wifi_connect(); // keep trying; BOOT-hold escapes bad creds
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = data;
        snprintf(ip_text, sizeof(ip_text), IPSTR, IP2STR(&e->ip_info.ip));
        got_ip = true;
        printf("D21_CONNECTED ip=%s\n", ip_text);
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

// --- Portal submit -> save + reboot ------------------------------------------
static void on_portal_submit(const char *ssid, const char *pass)
{
    printf("D21_SAVED ssid=%s\n", ssid);
    portal_save_creds(ssid, pass);
    vTaskDelay(pdMS_TO_TICKS(1500)); // let the phone render "Saved."
    esp_restart();
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

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Escape hatch: hold BOOT (GPIO0) during power-up to forget Wi-Fi.
    gpio_config_t boot_btn = {.pin_bit_mask = 1ULL << 0,
                              .mode = GPIO_MODE_INPUT,
                              .pull_up_en = GPIO_PULLUP_ENABLE};
    gpio_config(&boot_btn);
    if (!gpio_get_level(0)) {
        portal_clear_creds();
        printf("D21_RESET creds cleared\n");
    }

    char ssid[33] = {0}, pass[65] = {0};
    if (portal_load_creds(ssid, sizeof(ssid), pass, sizeof(pass))) {
        printf("D21_MODE station ssid=%s\n", ssid);
        ui_show("CONNECTING", ssid);
        station_start(ssid, pass);
        char body[96];
        while (1) {
            if (got_ip) {
                wifi_ap_record_t ap;
                esp_wifi_sta_get_ap_info(&ap);
                snprintf(body, sizeof(body), "%s\n%s\n%d dBm", ssid, ip_text,
                         ap.rssi);
                ui_show("ONLINE", body);
            } else {
                ui_show("CONNECTING", ssid);
            }
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    printf("D21_MODE portal\n");
    ui_show("SETUP",
            "Join Wi-Fi network\n\"esptember-setup\"\n\n"
            "the setup page opens\nby itself");
    portal_start(on_portal_submit);
}
