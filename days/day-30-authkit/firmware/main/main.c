// ESPtember Day 30 — WorkOS AuthKit, Device Authorization Grant
// The finale: a device with no keyboard and no browser signs a real
// user into a real identity provider. The OAuth 2.0 device flow (RFC
// 8628) is built for exactly this shape of hardware: the board asks
// WorkOS for a code pair, shows the human-readable code and a QR of the
// verification URL, and polls until the human approves on their phone.
// No secrets ship in this firmware: the client ID is public by design,
// entered once over serial, stored in NVS. Wi-Fi provisioning is
// day 21's portal, lifted whole. Board bring-up (pmu_init,
// panel_reset_release) carries over from day 01.
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
#include "esp_timer.h"
#include "portal.h"

#define AUTHORIZE_URL "https://api.workos.com/user_management/authorize/device"
#define TOKEN_URL "https://api.workos.com/user_management/authenticate"
#define GRANT_TYPE "urn:ietf:params:oauth:grant-type:device_code"

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

// --- HTTPS form POST -> JSON ----------------------------------------------------
static char http_body[4096];

static cJSON *post_form(const char *url, const char *form, int *status)
{
    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_header(client, "Content-Type",
                               "application/x-www-form-urlencoded");
    esp_http_client_open(client, strlen(form));
    esp_http_client_write(client, form, strlen(form));
    esp_http_client_fetch_headers(client);
    const int len = esp_http_client_read_response(client, http_body,
                                                  sizeof(http_body) - 1);
    *status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (len <= 0) return NULL;
    http_body[len] = 0;
    return cJSON_Parse(http_body);
}

// --- UI --------------------------------------------------------------------------
#define ESPTEMBER_ORANGE 0xff5b04

static lv_obj_t *title_label;
static lv_obj_t *code_label;
static lv_obj_t *body_label;
static lv_obj_t *qr;

static void ui_state(const char *title, const char *code, const char *body,
                     const char *qr_url)
{
    bsp_display_lock(0);
    lv_label_set_text(title_label, title);
    lv_label_set_text(code_label, code);
    lv_label_set_text(body_label, body);
    if (qr_url) {
        lv_qrcode_update(qr, qr_url, strlen(qr_url));
        lv_obj_remove_flag(qr, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(qr, LV_OBJ_FLAG_HIDDEN);
    }
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
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
    lv_label_set_text(title_label, "AUTHKIT");

    qr = lv_qrcode_create(screen);
    lv_qrcode_set_size(qr, 190);
    lv_qrcode_set_dark_color(qr, lv_color_black());
    lv_qrcode_set_light_color(qr, lv_color_white());
    lv_obj_align(qr, LV_ALIGN_TOP_MID, 0, 64);
    lv_obj_add_flag(qr, LV_OBJ_FLAG_HIDDEN);

    code_label = lv_label_create(screen);
    lv_obj_set_style_text_color(code_label, lv_color_white(), 0);
    lv_obj_align(code_label, LV_ALIGN_TOP_MID, 0, 280);
    lv_label_set_text(code_label, "");

    body_label = lv_label_create(screen);
    lv_obj_set_width(body_label, lv_pct(90));
    lv_obj_set_style_text_align(body_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(body_label, lv_color_hex(0x888888), 0);
    lv_obj_align(body_label, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_label_set_text(body_label, "");
}

// --- Client id storage ------------------------------------------------------------
static char client_id[64] = "";

static bool load_client_id(void)
{
    nvs_handle_t nvs;
    if (nvs_open("authkit", NVS_READONLY, &nvs) != ESP_OK) return false;
    size_t len = sizeof(client_id);
    const bool ok = nvs_get_str(nvs, "client_id", client_id, &len) == ESP_OK &&
                    client_id[0];
    nvs_close(nvs);
    return ok;
}

static void save_client_id(const char *id)
{
    nvs_handle_t nvs;
    ESP_ERROR_CHECK(nvs_open("authkit", NVS_READWRITE, &nvs));
    ESP_ERROR_CHECK(nvs_set_str(nvs, "client_id", id));
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);
}

// --- The device flow ---------------------------------------------------------------
static void device_flow_task(void *arg)
{
    (void)arg;
    while (1) {
        // Step 1: ask WorkOS for a code pair.
        char form[128];
        snprintf(form, sizeof(form), "client_id=%s", client_id);
        int status = 0;
        cJSON *r = post_form(AUTHORIZE_URL, form, &status);
        if (!r || status != 200) {
            printf("D30_AUTHORIZE_FAILED http=%d body=%.200s\n", status,
                   http_body);
            ui_state("AUTHKIT", "", "authorize failed - check client id", NULL);
            if (r) cJSON_Delete(r);
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }
        char device_code[128] = "", user_code[16] = "", uri[256] = "";
        int interval = 5, expires = 300;
        cJSON *v;
        if ((v = cJSON_GetObjectItem(r, "device_code")) && cJSON_IsString(v))
            strlcpy(device_code, v->valuestring, sizeof(device_code));
        if ((v = cJSON_GetObjectItem(r, "user_code")) && cJSON_IsString(v))
            strlcpy(user_code, v->valuestring, sizeof(user_code));
        if ((v = cJSON_GetObjectItem(r, "verification_uri_complete")) &&
            cJSON_IsString(v))
            strlcpy(uri, v->valuestring, sizeof(uri));
        if ((v = cJSON_GetObjectItem(r, "interval")) && cJSON_IsNumber(v))
            interval = v->valueint;
        if ((v = cJSON_GetObjectItem(r, "expires_in")) && cJSON_IsNumber(v))
            expires = v->valueint;
        cJSON_Delete(r);
        printf("D30_CODE user_code=%s uri=%s interval=%d\n", user_code, uri,
               interval);

        // Step 2: show the human their half of the job.
        ui_state("SIGN IN", user_code, "scan, then approve on your phone",
                 uri);

        // Step 3: poll until approved, denied, or expired. The interval
        // is the server's, not ours — RFC 8628 calls hammering a
        // slow_down offense.
        const int64_t deadline = esp_timer_get_time() + (int64_t)expires * 1000000;
        char poll_form[320];
        snprintf(poll_form, sizeof(poll_form),
                 "grant_type=" GRANT_TYPE "&client_id=%s&device_code=%s",
                 client_id, device_code);
        bool done = false;
        while (!done && esp_timer_get_time() < deadline) {
            vTaskDelay(pdMS_TO_TICKS(interval * 1000));
            r = post_form(TOKEN_URL, poll_form, &status);
            if (!r) continue;
            if (status == 200) {
                const cJSON *user = cJSON_GetObjectItem(r, "user");
                const cJSON *email =
                    user ? cJSON_GetObjectItem(user, "email") : NULL;
                char body[96];
                snprintf(body, sizeof(body), "signed in as\n%s",
                         email && cJSON_IsString(email) ? email->valuestring
                                                        : "(unknown)");
                ui_state("AUTHORIZED", "", body, NULL);
                printf("D30_AUTHORIZED email=%s\n",
                       email && cJSON_IsString(email) ? email->valuestring
                                                      : "?");
                done = true;
            } else {
                const cJSON *err = cJSON_GetObjectItem(r, "error");
                const char *code =
                    err && cJSON_IsString(err) ? err->valuestring : "";
                if (!strcmp(code, "slow_down")) interval += 5;
                else if (strcmp(code, "authorization_pending")) {
                    printf("D30_DENIED error=%s\n", code);
                    ui_state("DENIED", "", code, NULL);
                    done = true;
                }
            }
            cJSON_Delete(r);
        }
        if (done) vTaskDelete(NULL); // stay on the final screen
        printf("D30_EXPIRED\n");     // codes rot; fetch a fresh pair
    }
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

// Serial config: `client client_01XXXX` sets the public client id.
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
        char id[64];
        if (sscanf(line, "client %63s", id) == 1) {
            save_client_id(id);
            printf("D30_CLIENT saved=%s\n", id);
            esp_restart();
        } else if (!strcmp(line, "s")) {
            printf("D30_STATUS wifi=%d client=%s\n", (int)got_ip,
                   client_id[0] ? client_id : "(unset)");
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
    xTaskCreate(console_task, "console", 4096, NULL, 5, NULL);

    char ssid[33] = {0}, pass[65] = {0};
    if (!portal_load_creds(ssid, sizeof(ssid), pass, sizeof(pass))) {
        ui_state("SETUP", "",
                 "join Wi-Fi network\n\"esptember-setup\"", NULL);
        printf("D30_MODE portal\n");
        portal_start(on_portal_submit);
        return;
    }

    if (!load_client_id()) {
        ui_state("AUTHKIT", "",
                 "set your public client id:\nserial: client client_01...",
                 NULL);
        printf("D30_MODE need-client-id\n");
        return; // console task handles the save + restart
    }

    ui_state("AUTHKIT", "", "connecting...", NULL);
    station_start(ssid, pass);
    while (!got_ip) vTaskDelay(pdMS_TO_TICKS(200));
    printf("D30_READY client=%s\n", client_id);
    xTaskCreate(device_flow_task, "device_flow", 8192, NULL, 5, NULL);
}
