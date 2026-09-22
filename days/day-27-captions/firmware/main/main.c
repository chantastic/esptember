// ESPtember Day 27 — Real-Time Captions
// The mic streams to Deepgram's live WebSocket and words come back
// while you're still saying them. The screen shows finalized lines in
// white and the interim hypothesis in gray — watching the gray line
// rewrite itself as context arrives is the whole show. Key over serial
// (`key YOUR_DEEPGRAM_KEY`), Wi-Fi via day 21's portal, audio path from
// day 14, WebSocket discipline from day 24. Board bring-up carries over
// from day 01.
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_websocket_client.h"
#include "esp_crt_bundle.h"
#include "esp_codec_dev.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "portal.h"

#define SAMPLE_RATE 16000
#define CHUNK_SAMPLES 3200 // 200 ms per WebSocket message
#define DG_URI                                                              \
    "wss://api.deepgram.com/v1/listen?encoding=linear16&sample_rate=16000&" \
    "channels=1&interim_results=true&smart_format=true"

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

// --- Transcript state ---------------------------------------------------------
#define FINAL_LINES 6
static char final_lines[FINAL_LINES][64];
static int final_count = 0;
static char interim[128] = "";
static volatile bool transcript_dirty = false;
static volatile bool ws_up = false;
static volatile uint32_t results = 0;
static char api_key[48] = "";

static void push_final(const char *text)
{
    if (!text[0]) return;
    if (final_count == FINAL_LINES) {
        memmove(final_lines[0], final_lines[1],
                (FINAL_LINES - 1) * sizeof(final_lines[0]));
        final_count--;
    }
    strlcpy(final_lines[final_count++], text, sizeof(final_lines[0]));
}

// --- Deepgram response parsing ---------------------------------------------------
static void handle_result(const char *json, int len)
{
    cJSON *r = cJSON_ParseWithLength(json, len);
    if (!r) return;
    const cJSON *channel = cJSON_GetObjectItem(r, "channel");
    const cJSON *alts =
        channel ? cJSON_GetObjectItem(channel, "alternatives") : NULL;
    const cJSON *alt = alts ? cJSON_GetArrayItem(alts, 0) : NULL;
    const cJSON *transcript =
        alt ? cJSON_GetObjectItem(alt, "transcript") : NULL;
    const cJSON *is_final = cJSON_GetObjectItem(r, "is_final");
    if (cJSON_IsString(transcript)) {
        results++;
        if (cJSON_IsTrue(is_final)) {
            push_final(transcript->valuestring);
            interim[0] = 0;
            printf("D27_FINAL %s\n", transcript->valuestring);
        } else {
            strlcpy(interim, transcript->valuestring, sizeof(interim));
        }
        transcript_dirty = true;
    }
    cJSON_Delete(r);
}

// --- WebSocket -------------------------------------------------------------------
static esp_websocket_client_handle_t ws;

static void ws_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base;
    const esp_websocket_event_data_t *ev = data;
    if (id == WEBSOCKET_EVENT_CONNECTED) {
        ws_up = true;
        printf("D27_WS connected\n");
    } else if (id == WEBSOCKET_EVENT_DISCONNECTED) {
        ws_up = false;
    } else if (id == WEBSOCKET_EVENT_DATA && ev->op_code == 1) {
        handle_result(ev->data_ptr, ev->data_len);
    }
}

static void ws_start(void)
{
    static char auth_header[80];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Token %s\r\n",
             api_key);
    const esp_websocket_client_config_t cfg = {
        .uri = DG_URI,
        .headers = auth_header,
        .reconnect_timeout_ms = 3000,
        .network_timeout_ms = 10000,
        .buffer_size = 4096,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    ws = esp_websocket_client_init(&cfg);
    esp_websocket_register_events(ws, WEBSOCKET_EVENT_ANY, ws_event, NULL);
    esp_websocket_client_start(ws);
}

// --- Mic -> WebSocket ---------------------------------------------------------------
static int16_t chunk[CHUNK_SAMPLES];

static void mic_task(void *arg)
{
    esp_codec_dev_handle_t mic = (esp_codec_dev_handle_t)arg;
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = SAMPLE_RATE,
        .channel = 1,
        .bits_per_sample = 16,
    };
    esp_codec_dev_open(mic, &fs);
    esp_codec_dev_set_in_gain(mic, 30.0);
    while (1) {
        // The mic paces the stream, as in day 23: 200 ms per read, sent
        // whole. Deepgram accepts raw linear16 binary messages.
        esp_codec_dev_read(mic, chunk, sizeof(chunk));
        if (ws_up)
            esp_websocket_client_send_bin(ws, (const char *)chunk,
                                          sizeof(chunk), portMAX_DELAY);
    }
}

// --- UI -----------------------------------------------------------------------------
#define ESPTEMBER_ORANGE 0xff5b04

static lv_obj_t *final_label;
static lv_obj_t *interim_label;
static lv_obj_t *state_label;

static void ui_tick(lv_timer_t *t)
{
    (void)t;
    if (!transcript_dirty) return;
    transcript_dirty = false;
    static char joined[FINAL_LINES * 64];
    joined[0] = 0;
    for (int i = 0; i < final_count; i++) {
        strlcat(joined, final_lines[i], sizeof(joined));
        strlcat(joined, "\n", sizeof(joined));
    }
    lv_label_set_text(final_label, joined);
    lv_label_set_text(interim_label, interim);
}

static void state_tick(lv_timer_t *t)
{
    (void)t;
    lv_label_set_text(state_label, !api_key[0] ? "serial: key DEEPGRAM_KEY"
                                   : ws_up     ? "listening"
                                               : "connecting...");
}

static void build_ui(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_text_font(screen, &lv_font_montserrat_28, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "CAPTIONS");
    lv_obj_set_style_text_color(title, lv_color_hex(ESPTEMBER_ORANGE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    final_label = lv_label_create(screen);
    lv_obj_set_width(final_label, lv_pct(94));
    lv_label_set_long_mode(final_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(final_label, LV_ALIGN_TOP_LEFT, 12, 56);
    lv_label_set_text(final_label, "");

    interim_label = lv_label_create(screen);
    lv_obj_set_width(interim_label, lv_pct(94));
    lv_label_set_long_mode(interim_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(interim_label, lv_color_hex(0x777777), 0);
    lv_obj_align(interim_label, LV_ALIGN_BOTTOM_LEFT, 12, -60);
    lv_label_set_text(interim_label, "");

    state_label = lv_label_create(screen);
    lv_obj_set_style_text_color(state_label, lv_color_hex(0x555555), 0);
    lv_obj_align(state_label, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_label_set_text(state_label, "");

    lv_timer_create(ui_tick, 100, NULL);
    lv_timer_create(state_tick, 500, NULL);
}

// --- Key storage / console -------------------------------------------------------
static bool load_key(void)
{
    nvs_handle_t nvs;
    if (nvs_open("day27", NVS_READONLY, &nvs) != ESP_OK) return false;
    size_t len = sizeof(api_key);
    const bool ok =
        nvs_get_str(nvs, "key", api_key, &len) == ESP_OK && api_key[0];
    nvs_close(nvs);
    return ok;
}

static void console_task(void *arg)
{
    (void)arg;
    usb_serial_jtag_driver_config_t cfg =
        USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    usb_serial_jtag_driver_install(&cfg);
    usb_serial_jtag_vfs_use_driver();
    char line[128];
    while (fgets(line, sizeof(line), stdin)) {
        char *nl = strpbrk(line, "\r\n");
        if (nl) *nl = 0;
        char id[48];
        if (sscanf(line, "key %47s", id) == 1) {
            nvs_handle_t nvs;
            ESP_ERROR_CHECK(nvs_open("day27", NVS_READWRITE, &nvs));
            nvs_set_str(nvs, "key", id);
            nvs_commit(nvs);
            nvs_close(nvs);
            printf("D27_KEY saved\n");
            esp_restart();
        } else if (!strcmp(line, "s")) {
            printf("D27_STATUS ws=%d results=%lu finals=%d key=%d\n",
                   (int)ws_up, (unsigned long)results, final_count,
                   (int)(api_key[0] != 0));
        }
    }
    vTaskDelete(NULL);
}

// --- Wi-Fi (day 21/24 pattern) -----------------------------------------------------
static volatile bool got_ip = false;

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id,
                          void *data)
{
    (void)arg; (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) esp_wifi_connect();
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        got_ip = false;
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) got_ip = true;
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

void app_main(void)
{
    pmu_init();
    panel_reset_release();

    bsp_audio_init(NULL);
    esp_codec_dev_handle_t mic = bsp_audio_codec_microphone_init();

    bsp_display_start();
    bsp_display_backlight_on();
    bsp_display_lock(0);
    build_ui();
    bsp_display_unlock();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    load_key();
    xTaskCreate(console_task, "console", 6144, NULL, 5, NULL);

    char ssid[33] = {0}, pass[65] = {0};
    if (!portal_load_creds(ssid, sizeof(ssid), pass, sizeof(pass))) {
        printf("D27_MODE portal\n");
        portal_start(on_portal_submit);
        return;
    }
    station_start(ssid, pass);
    while (!got_ip) vTaskDelay(pdMS_TO_TICKS(200));
    printf("D27_READY key=%d\n", (int)(api_key[0] != 0));
    if (!api_key[0]) return; // console handles key entry + restart

    ws_start();
    xTaskCreate(mic_task, "mic", 6144, mic, 6, NULL);
}
