// ESPtember Day 33 — Pipes
// September doesn't have a 31st, and the series didn't have "use the
// identity for something" — both problems solved at once. Day 31
// proved who you are; today that identity does work: the board lists
// your Pipes providers (GitHub, Slack, Google...), shows which are
// connected, QRs the authorization URL for ones that aren't, and — for
// connected GitHub — calls the provider through Pipes Relay, where the
// provider token never exists on this device, in the gateway, or
// anywhere it could leak.
//
// The firmware is deliberately dumb: sign in, GET one /screen payload,
// render it. Connect URLs come pre-minted, provider details come
// pre-fetched (via Relay, inside the Worker) — features evolve by
// redeploying the gateway; this firmware never changes. The device's
// only credential is its AuthKit access token; the ~100-line Cloudflare
// Worker (gateway/) verifies it and holds the WorkOS key.
// The refresh token persists in NVS, so sign-in survives reboots.
// Board bring-up (pmu_init, panel_reset_release) carries over from
// day 01; Wi-Fi from day 22's portal; the device grant from day 31.
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
#define DEVICE_GRANT "urn:ietf:params:oauth:grant-type:device_code"
#define REFRESH_GRANT "refresh_token"
#define GATEWAY "https://esptember-pipes-gateway.chantastic.workers.dev"

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

// --- HTTP helpers ----------------------------------------------------------------
static char http_body[8192];

static cJSON *http_json(const char *url, const char *method,
                        const char *content_type, const char *payload,
                        const char *bearer, int *status)
{
    esp_http_client_config_t cfg = {
        .url = url,
        .method = !strcmp(method, "POST") ? HTTP_METHOD_POST : HTTP_METHOD_GET,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 30000, // the gateway composes many upstream calls
        // The Authorization header carries a ~1 KB JWT; the default
        // 512-byte header buffer silently mangles the request.
        .buffer_size = 2048,
        .buffer_size_tx = 4096,
    };
    http_body[0] = 0; // never let a failure show the previous call's body
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (content_type)
        esp_http_client_set_header(client, "Content-Type", content_type);
    if (bearer) {
        static char auth[1200];
        snprintf(auth, sizeof(auth), "Bearer %s", bearer);
        esp_http_client_set_header(client, "Authorization", auth);
    }
    esp_http_client_open(client, payload ? strlen(payload) : 0);
    if (payload) esp_http_client_write(client, payload, strlen(payload));
    esp_http_client_fetch_headers(client);
    const int len = esp_http_client_read_response(client, http_body,
                                                  sizeof(http_body) - 1);
    *status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (len <= 0) return NULL;
    http_body[len] = 0;
    return cJSON_Parse(http_body);
}

// --- Auth state -------------------------------------------------------------------
static char client_id[64] = "";
static char access_token[1100] = "";
static char user_email[64] = "";

static bool load_str(const char *ns, const char *key, char *out, size_t n)
{
    nvs_handle_t nvs;
    if (nvs_open(ns, NVS_READONLY, &nvs) != ESP_OK) return false;
    size_t len = n;
    const bool ok = nvs_get_str(nvs, key, out, &len) == ESP_OK && out[0];
    nvs_close(nvs);
    return ok;
}

static void save_str(const char *ns, const char *key, const char *val)
{
    nvs_handle_t nvs;
    ESP_ERROR_CHECK(nvs_open(ns, NVS_READWRITE, &nvs));
    ESP_ERROR_CHECK(nvs_set_str(nvs, key, val));
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);
}

// Absorb an authenticate response: tokens + email. The refresh token
// goes to NVS — WorkOS rotates it on every use, so always keep the
// newest one. Sign-in survives reboots because of this function.
static bool absorb_tokens(const cJSON *r)
{
    const cJSON *at = cJSON_GetObjectItem(r, "access_token");
    const cJSON *rt = cJSON_GetObjectItem(r, "refresh_token");
    if (!cJSON_IsString(at)) return false;
    strlcpy(access_token, at->valuestring, sizeof(access_token));
    if (cJSON_IsString(rt)) save_str("authkit", "refresh", rt->valuestring);
    const cJSON *user = cJSON_GetObjectItem(r, "user");
    const cJSON *email = user ? cJSON_GetObjectItem(user, "email") : NULL;
    if (cJSON_IsString(email))
        strlcpy(user_email, email->valuestring, sizeof(user_email));
    return true;
}

// --- UI ---------------------------------------------------------------------------
#define ESPTEMBER_ORANGE 0xff5b04

static lv_obj_t *title_label;
static lv_obj_t *code_label;
static lv_obj_t *body_label;
static lv_obj_t *qr;
static lv_obj_t *provider_list;

static void ui_state(const char *title, const char *code, const char *body,
                     const char *qr_url)
{
    bsp_display_lock(0);
    lv_label_set_text(title_label, title);
    lv_label_set_text(code_label, code);
    lv_label_set_text(body_label, body);
    lv_obj_add_flag(provider_list, LV_OBJ_FLAG_HIDDEN);
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
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 16);
    lv_label_set_text(title_label, "PIPES");

    qr = lv_qrcode_create(screen);
    lv_qrcode_set_size(qr, 190);
    lv_qrcode_set_dark_color(qr, lv_color_black());
    lv_qrcode_set_light_color(qr, lv_color_white());
    lv_obj_align(qr, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_add_flag(qr, LV_OBJ_FLAG_HIDDEN);

    code_label = lv_label_create(screen);
    lv_obj_align(code_label, LV_ALIGN_TOP_MID, 0, 272);
    lv_label_set_text(code_label, "");

    body_label = lv_label_create(screen);
    lv_obj_set_width(body_label, lv_pct(92));
    lv_obj_set_style_text_align(body_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(body_label, lv_color_hex(0x888888), 0);
    lv_obj_align(body_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_label_set_text(body_label, "");

    provider_list = lv_list_create(screen);
    lv_obj_set_size(provider_list, lv_pct(96), 330);
    lv_obj_align(provider_list, LV_ALIGN_TOP_MID, 0, 56);
    lv_obj_set_style_bg_color(provider_list, lv_color_black(), 0);
    lv_obj_set_style_border_width(provider_list, 0, 0);
    lv_obj_add_flag(provider_list, LV_OBJ_FLAG_HIDDEN);
}

// --- The screen, as served -----------------------------------------------------
typedef struct {
    char slug[24];
    char detail[48];       // connected: one human line, pre-fetched
    char connect_url[256]; // unconnected: pre-minted, ready to QR
    bool connected;
} provider_t;

#define MAX_PROVIDERS 12
static provider_t providers[MAX_PROVIDERS];
static int provider_count = 0;
static volatile int pending_action = -1; // provider index tapped

static void row_cb(lv_event_t *e)
{
    pending_action = (int)(intptr_t)lv_event_get_user_data(e);
}

static void show_provider_list(const char *note)
{
    bsp_display_lock(0);
    lv_obj_add_flag(qr, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(title_label, "PIPES");
    lv_label_set_text(code_label, "");
    lv_label_set_text(body_label, note ? note : user_email);
    lv_obj_clean(provider_list);
    for (int i = 0; i < provider_count; i++) {
        char row[80];
        if (providers[i].connected && providers[i].detail[0])
            snprintf(row, sizeof(row), "%s  \xC2\xB7  %s", providers[i].slug,
                     providers[i].detail);
        else
            snprintf(row, sizeof(row), "%s", providers[i].slug);
        lv_obj_t *btn = lv_list_add_button(
            provider_list,
            providers[i].connected ? LV_SYMBOL_OK : LV_SYMBOL_PLUS, row);
        lv_obj_set_style_bg_color(btn, lv_color_black(), 0);
        lv_obj_set_style_text_color(btn,
                                    providers[i].connected
                                        ? lv_color_hex(0x00e676)
                                        : lv_color_hex(0x888888),
                                    0);
        lv_obj_set_style_pad_ver(btn, 16, 0);
        lv_obj_add_event_cb(btn, row_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
    }
    lv_obj_remove_flag(provider_list, LV_OBJ_FLAG_HIDDEN);
    bsp_display_unlock();
}

// One request; the response IS the interface.
static bool fetch_screen(void)
{
    int status = 0;
    cJSON *r = http_json(GATEWAY "/screen", "GET", NULL, NULL, access_token,
                         &status);
    if (!r || status != 200) {
        printf("D31_SCREEN_FAILED http=%d body=%.200s\n", status, http_body);
        if (r) cJSON_Delete(r);
        return false;
    }
    provider_count = 0;
    const cJSON *list = cJSON_GetObjectItem(r, "providers");
    cJSON *item;
    cJSON_ArrayForEach(item, list) {
        if (provider_count >= MAX_PROVIDERS) break;
        provider_t *p = &providers[provider_count];
        memset(p, 0, sizeof(*p));
        const cJSON *v;
        if (!cJSON_IsString(v = cJSON_GetObjectItem(item, "slug"))) continue;
        strlcpy(p->slug, v->valuestring, sizeof(p->slug));
        p->connected = cJSON_IsTrue(cJSON_GetObjectItem(item, "connected"));
        if (cJSON_IsString(v = cJSON_GetObjectItem(item, "detail")))
            strlcpy(p->detail, v->valuestring, sizeof(p->detail));
        if (cJSON_IsString(v = cJSON_GetObjectItem(item, "connect_url")))
            strlcpy(p->connect_url, v->valuestring, sizeof(p->connect_url));
        printf("D31_PROVIDER %s connected=%d detail=%s\n", p->slug,
               p->connected, p->detail);
        provider_count++;
    }
    cJSON_Delete(r);
    return true;
}

// Tap an unconnected provider: QR the URL the gateway already minted,
// then poll the screen until the connection appears.
static void connect_provider(int index)
{
    if (!providers[index].connect_url[0]) {
        show_provider_list("no connect URL");
        return;
    }
    char note[48];
    snprintf(note, sizeof(note), "scan to connect %s", providers[index].slug);
    char slug[24];
    strlcpy(slug, providers[index].slug, sizeof(slug));
    ui_state("CONNECT", "", note, providers[index].connect_url);
    printf("D31_CONNECT %s\n", slug);
    for (int i = 0; i < 36; i++) { // ~3 minutes of patience
        vTaskDelay(pdMS_TO_TICKS(5000));
        if (!fetch_screen()) continue;
        for (int p = 0; p < provider_count; p++) {
            if (!strcmp(providers[p].slug, slug) && providers[p].connected) {
                printf("D31_CONNECTED %s\n", slug);
                show_provider_list("connected!");
                return;
            }
        }
    }
    show_provider_list("not connected (timed out)");
}

// --- Sign-in (day 31, plus refresh persistence) -------------------------------------
static bool try_refresh(void)
{
    char refresh[600];
    if (!load_str("authkit", "refresh", refresh, sizeof(refresh)))
        return false;
    char form[900];
    snprintf(form, sizeof(form),
             "grant_type=" REFRESH_GRANT "&client_id=%s&refresh_token=%s",
             client_id, refresh);
    int status = 0;
    cJSON *r = http_json(TOKEN_URL, "POST",
                         "application/x-www-form-urlencoded", form, NULL,
                         &status);
    const bool ok = r && status == 200 && absorb_tokens(r);
    if (r) cJSON_Delete(r);
    printf("D31_REFRESH %s\n", ok ? "ok" : "failed");
    return ok;
}

static bool device_flow(void)
{
    char form[128];
    snprintf(form, sizeof(form), "client_id=%s", client_id);
    int status = 0;
    cJSON *r = http_json(AUTHORIZE_URL, "POST",
                         "application/x-www-form-urlencoded", form, NULL,
                         &status);
    if (!r || status != 200) {
        if (r) cJSON_Delete(r);
        ui_state("PIPES", "", "authorize failed - check client id", NULL);
        return false;
    }
    char device_code[128] = "", user_code[16] = "", uri[256] = "";
    int interval = 5;
    const cJSON *v;
    if ((v = cJSON_GetObjectItem(r, "device_code")) && cJSON_IsString(v))
        strlcpy(device_code, v->valuestring, sizeof(device_code));
    if ((v = cJSON_GetObjectItem(r, "user_code")) && cJSON_IsString(v))
        strlcpy(user_code, v->valuestring, sizeof(user_code));
    if ((v = cJSON_GetObjectItem(r, "verification_uri_complete")) &&
        cJSON_IsString(v))
        strlcpy(uri, v->valuestring, sizeof(uri));
    if ((v = cJSON_GetObjectItem(r, "interval")) && cJSON_IsNumber(v))
        interval = v->valueint;
    cJSON_Delete(r);
    printf("D31_CODE user_code=%s\n", user_code);
    ui_state("SIGN IN", user_code, "scan, then approve on your phone", uri);

    char poll[360];
    snprintf(poll, sizeof(poll),
             "grant_type=" DEVICE_GRANT "&client_id=%s&device_code=%s",
             client_id, device_code);
    for (int i = 0; i < 60; i++) {
        vTaskDelay(pdMS_TO_TICKS(interval * 1000));
        r = http_json(TOKEN_URL, "POST", "application/x-www-form-urlencoded",
                      poll, NULL, &status);
        if (!r) continue;
        if (status == 200 && absorb_tokens(r)) {
            cJSON_Delete(r);
            printf("D31_AUTHORIZED email=%s\n", user_email);
            return true;
        }
        const cJSON *err = cJSON_GetObjectItem(r, "error");
        if (cJSON_IsString(err) && !strcmp(err->valuestring, "slow_down"))
            interval += 5;
        cJSON_Delete(r);
    }
    return false;
}

// --- Main loop task ------------------------------------------------------------------
static void app_task(void *arg)
{
    (void)arg;
    if (!try_refresh() && !device_flow()) {
        ui_state("PIPES", "", "sign-in failed", NULL);
        vTaskDelete(NULL);
    }
    ui_state("PIPES", "", "loading...", NULL);
    bool loaded = false;
    for (int attempt = 0; attempt < 3 && !loaded; attempt++) {
        loaded = fetch_screen();
        if (!loaded) vTaskDelay(pdMS_TO_TICKS(2000));
    }
    if (!loaded) {
        ui_state("PIPES", "", "gateway unreachable -\nis the secret set?",
                 NULL);
        vTaskDelete(NULL);
    }
    show_provider_list(NULL);

    int64_t last_refresh = esp_timer_get_time();
    while (1) {
        if (pending_action >= 0 && pending_action < provider_count) {
            const int i = pending_action;
            pending_action = -1;
            if (!providers[i].connected) connect_provider(i);
            // connected rows already display their detail; nothing to do
        }
        // The screen refreshes itself every minute — server-driven UI.
        if (esp_timer_get_time() - last_refresh > 60LL * 1000000) {
            last_refresh = esp_timer_get_time();
            if (fetch_screen()) show_provider_list(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// --- Wi-Fi + console (day 31's pattern) ----------------------------------------------
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
            save_str("authkit", "client_id", id);
            printf("D31_CLIENT saved\n");
            esp_restart();
        } else if (!strcmp(line, "signout")) {
            nvs_handle_t nvs;
            if (nvs_open("authkit", NVS_READWRITE, &nvs) == ESP_OK) {
                nvs_erase_key(nvs, "refresh");
                nvs_commit(nvs);
                nvs_close(nvs);
            }
            printf("D31_SIGNOUT\n");
            esp_restart();
        } else if (!strcmp(line, "s")) {
            printf("D31_STATUS wifi=%d email=%s providers=%d\n", (int)got_ip,
                   user_email[0] ? user_email : "(none)", provider_count);
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
    xTaskCreate(console_task, "console", 6144, NULL, 5, NULL);

    char ssid[33] = {0}, pass[65] = {0};
    if (!portal_load_creds(ssid, sizeof(ssid), pass, sizeof(pass))) {
        ui_state("SETUP", "", "join Wi-Fi network\n\"esptember-setup\"",
                 NULL);
        portal_start(on_portal_submit);
        return;
    }
    if (!load_str("authkit", "client_id", client_id, sizeof(client_id))) {
        ui_state("PIPES", "",
                 "set your client id:\nserial: client client_01...", NULL);
        return;
    }
    ui_state("PIPES", "", "connecting...", NULL);
    station_start(ssid, pass);
    while (!got_ip) vTaskDelay(pdMS_TO_TICKS(200));
    printf("D31_READY\n");
    xTaskCreate(app_task, "app", 12288, NULL, 5, NULL);
}
