// ESPtember Day 26 — Package Tracker
// The first *keyed* API of the series: 17TRACK's free tier follows any
// carrier from one tracking number. The key is provisioned like every
// secret this month — over serial, into NVS, never into the repo:
//   key  YOUR_17TRACK_KEY
//   add  TRACKINGNUMBER      (up to 6)
//   del  TRACKINGNUMBER
// Wi-Fi arrives through day 21's portal. The screen is a status board:
// one row per package, latest event line, color by delivery state.
// Board bring-up (pmu_init, panel_reset_release) carries over from
// day 01.
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
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "portal.h"

#define API_REGISTER "https://api.17track.net/track/v2.2/register"
#define API_GETINFO "https://api.17track.net/track/v2.2/gettrackinfo"
#define MAX_PACKAGES 6
#define REFRESH_MS (15 * 60 * 1000) // free tier is a budget: 15 min

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

// --- Package store ------------------------------------------------------------
typedef struct {
    char number[36];
    char status[16];  // NotFound / InTransit / Delivered / ...
    char event[64];   // latest tracking event text
    bool registered;  // 17track wants numbers registered before queries
} package_t;

static package_t packages[MAX_PACKAGES];
static int package_count = 0;
static char api_key[48] = "";
static char status_line[40] = "starting";

static void store_save(void)
{
    nvs_handle_t nvs;
    ESP_ERROR_CHECK(nvs_open("day26", NVS_READWRITE, &nvs));
    nvs_set_str(nvs, "key", api_key);
    nvs_set_blob(nvs, "pkgs", packages, sizeof(packages));
    nvs_set_i32(nvs, "count", package_count);
    nvs_commit(nvs);
    nvs_close(nvs);
}

static void store_load(void)
{
    nvs_handle_t nvs;
    if (nvs_open("day26", NVS_READONLY, &nvs) != ESP_OK) return;
    size_t len = sizeof(api_key);
    nvs_get_str(nvs, "key", api_key, &len);
    len = sizeof(packages);
    nvs_get_blob(nvs, "pkgs", packages, &len);
    int32_t n = 0;
    nvs_get_i32(nvs, "count", &n);
    package_count = n;
    nvs_close(nvs);
}

// --- 17TRACK client -------------------------------------------------------------
static char http_body[8192];

static cJSON *post_json(const char *url, const char *payload, int *status)
{
    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "17token", api_key);
    esp_http_client_open(client, strlen(payload));
    esp_http_client_write(client, payload, strlen(payload));
    esp_http_client_fetch_headers(client);
    const int len = esp_http_client_read_response(client, http_body,
                                                  sizeof(http_body) - 1);
    *status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (len <= 0) return NULL;
    http_body[len] = 0;
    return cJSON_Parse(http_body);
}

static void register_new_numbers(void)
{
    for (int i = 0; i < package_count; i++) {
        if (packages[i].registered) continue;
        char payload[96];
        snprintf(payload, sizeof(payload), "[{\"number\":\"%.35s\"}]",
                 packages[i].number);
        int status = 0;
        cJSON *r = post_json(API_REGISTER, payload, &status);
        // "already registered" is also success for our purposes.
        packages[i].registered = (status == 200);
        printf("D26_REGISTER %s http=%d\n", packages[i].number, status);
        if (r) cJSON_Delete(r);
    }
    store_save();
}

static void refresh(void)
{
    if (!api_key[0] || !package_count) return;
    register_new_numbers();

    char payload[MAX_PACKAGES * 48 + 8] = "[";
    for (int i = 0; i < package_count; i++) {
        char item[64];
        snprintf(item, sizeof(item), "%s{\"number\":\"%.35s\"}", i ? "," : "",
                 packages[i].number);
        strlcat(payload, item, sizeof(payload));
    }
    strlcat(payload, "]", sizeof(payload));

    int status = 0;
    cJSON *r = post_json(API_GETINFO, payload, &status);
    if (!r || status != 200) {
        snprintf(status_line, sizeof(status_line), "HTTP %d", status);
        printf("D26_FETCH_FAILED http=%d\n", status);
        if (r) cJSON_Delete(r);
        return;
    }
    const cJSON *accepted =
        cJSON_GetObjectItem(cJSON_GetObjectItem(r, "data"), "accepted");
    cJSON *item;
    cJSON_ArrayForEach(item, accepted) {
        const cJSON *num = cJSON_GetObjectItem(item, "number");
        if (!cJSON_IsString(num)) continue;
        for (int i = 0; i < package_count; i++) {
            if (strcmp(packages[i].number, num->valuestring)) continue;
            const cJSON *info = cJSON_GetObjectItem(item, "track_info");
            const cJSON *latest =
                info ? cJSON_GetObjectItem(info, "latest_status") : NULL;
            const cJSON *st =
                latest ? cJSON_GetObjectItem(latest, "status") : NULL;
            if (cJSON_IsString(st))
                strlcpy(packages[i].status, st->valuestring,
                        sizeof(packages[i].status));
            const cJSON *ev =
                info ? cJSON_GetObjectItem(info, "latest_event") : NULL;
            const cJSON *desc =
                ev ? cJSON_GetObjectItem(ev, "description") : NULL;
            if (cJSON_IsString(desc))
                strlcpy(packages[i].event, desc->valuestring,
                        sizeof(packages[i].event));
            printf("D26_PACKAGE %s status=%s event=%s\n", packages[i].number,
                   packages[i].status, packages[i].event);
        }
    }
    cJSON_Delete(r);
    snprintf(status_line, sizeof(status_line), "ok");
    store_save();
}

// --- UI ----------------------------------------------------------------------------
#define ESPTEMBER_ORANGE 0xff5b04

static lv_obj_t *list_container;
static lv_obj_t *footer_label;

static lv_color_t status_color(const char *status)
{
    if (!strcmp(status, "Delivered")) return lv_color_hex(0x00c853);
    if (!strcmp(status, "OutForDelivery")) return lv_color_hex(0x00e5ff);
    if (!strcmp(status, "InTransit")) return lv_color_hex(ESPTEMBER_ORANGE);
    if (!strcmp(status, "Exception") || !strcmp(status, "Expired"))
        return lv_color_hex(0xff1744);
    return lv_color_hex(0x888888);
}

static void ui_refresh(void)
{
    bsp_display_lock(0);
    lv_obj_clean(list_container);
    if (!api_key[0]) {
        lv_obj_t *hint = lv_label_create(list_container);
        lv_label_set_text(hint, "serial: key YOUR_17TRACK_KEY");
        lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
    } else if (!package_count) {
        lv_obj_t *hint = lv_label_create(list_container);
        lv_label_set_text(hint, "serial: add TRACKINGNUMBER");
        lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
    }
    for (int i = 0; i < package_count; i++) {
        lv_obj_t *row = lv_obj_create(list_container);
        lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x101010), 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 10, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(row, 4, 0);

        lv_obj_t *num = lv_label_create(row);
        lv_label_set_text(num, packages[i].number);
        lv_obj_set_style_text_color(num, lv_color_white(), 0);
        lv_label_set_long_mode(num, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(num, lv_pct(100));

        lv_obj_t *st = lv_label_create(row);
        lv_label_set_text(st, packages[i].status[0] ? packages[i].status
                                                    : "pending");
        lv_obj_set_style_text_color(st, status_color(packages[i].status), 0);

        if (packages[i].event[0]) {
            lv_obj_t *ev = lv_label_create(row);
            lv_label_set_text(ev, packages[i].event);
            lv_obj_set_style_text_color(ev, lv_color_hex(0x888888), 0);
            lv_obj_set_width(ev, lv_pct(100));
            lv_label_set_long_mode(ev, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_font(ev, &lv_font_montserrat_14, 0);
        }
    }
    lv_label_set_text_fmt(footer_label, "%d tracked  ·  %s", package_count,
                          status_line);
    bsp_display_unlock();
}

static void build_ui(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_text_font(screen, &lv_font_montserrat_28, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "PACKAGES");
    lv_obj_set_style_text_color(title, lv_color_hex(ESPTEMBER_ORANGE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    list_container = lv_obj_create(screen);
    lv_obj_set_size(list_container, lv_pct(96), 330);
    lv_obj_align(list_container, LV_ALIGN_TOP_MID, 0, 58);
    lv_obj_set_style_bg_opa(list_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list_container, 0, 0);
    lv_obj_set_flex_flow(list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list_container, 8, 0);

    footer_label = lv_label_create(screen);
    lv_obj_set_style_text_color(footer_label, lv_color_hex(0x888888), 0);
    lv_obj_align(footer_label, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_label_set_text(footer_label, "");
}

// --- Wi-Fi (day 21/24 pattern) ------------------------------------------------------
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

// --- Console: key / add / del / refresh / status ------------------------------------
static volatile bool refresh_requested = false;

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
        char arg1[64];
        if (sscanf(line, "key %47s", api_key) == 1) {
            store_save();
            printf("D26_KEY saved\n");
            ui_refresh();
            refresh_requested = true;
        } else if (sscanf(line, "add %35s", arg1) == 1 &&
                   package_count < MAX_PACKAGES) {
            package_t *p = &packages[package_count++];
            memset(p, 0, sizeof(*p));
            strlcpy(p->number, arg1, sizeof(p->number));
            store_save();
            printf("D26_ADDED %s\n", arg1);
            ui_refresh();
            refresh_requested = true;
        } else if (sscanf(line, "del %35s", arg1) == 1) {
            for (int i = 0; i < package_count; i++) {
                if (strcmp(packages[i].number, arg1)) continue;
                memmove(&packages[i], &packages[i + 1],
                        (package_count - i - 1) * sizeof(package_t));
                package_count--;
                break;
            }
            store_save();
            printf("D26_DELETED %s\n", arg1);
            ui_refresh();
        } else if (!strcmp(line, "refresh")) {
            refresh_requested = true;
        } else if (!strcmp(line, "s")) {
            printf("D26_STATUS wifi=%d key=%d packages=%d status=%s\n",
                   (int)got_ip, (int)(api_key[0] != 0), package_count,
                   status_line);
        }
    }
    vTaskDelete(NULL);
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
    store_load();
    ui_refresh();
    xTaskCreate(console_task, "console", 6144, NULL, 5, NULL);

    char ssid[33] = {0}, pass[65] = {0};
    if (!portal_load_creds(ssid, sizeof(ssid), pass, sizeof(pass))) {
        bsp_display_lock(0);
        lv_label_set_text(footer_label, "join \"esptember-setup\" to begin");
        bsp_display_unlock();
        printf("D26_MODE portal\n");
        portal_start(on_portal_submit);
        return;
    }
    station_start(ssid, pass);
    printf("D26_READY\n");

    uint32_t last = 0;
    while (1) {
        const uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (got_ip && api_key[0] && package_count &&
            (refresh_requested || !last || now - last >= REFRESH_MS)) {
            refresh_requested = false;
            last = now;
            refresh();
            ui_refresh();
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
