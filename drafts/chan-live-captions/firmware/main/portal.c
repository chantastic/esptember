// Day 21 — the portal machinery: SoftAP, DNS hijack, HTTP form.
// The trick that makes phones pop the setup page automatically is the
// DNS server: it answers EVERY query with our own address. The phone
// probes a known URL to test connectivity, reaches us instead, sees an
// unexpected page, and offers it as a captive portal. That's the whole
// mechanism — a liar's DNS and an HTTP form.
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lwip/sockets.h"
#include "portal.h"

#define AP_SSID "esptember-setup"
#define AP_IP "192.168.4.1"

// --- Credential storage ------------------------------------------------------
bool portal_load_creds(char *ssid, size_t ssid_len, char *pass,
                       size_t pass_len)
{
    nvs_handle_t nvs;
    if (nvs_open("wifi", NVS_READONLY, &nvs) != ESP_OK) return false;
    esp_err_t a = nvs_get_str(nvs, "ssid", ssid, &ssid_len);
    esp_err_t b = nvs_get_str(nvs, "pass", pass, &pass_len);
    nvs_close(nvs);
    return a == ESP_OK && b == ESP_OK && ssid[0];
}

void portal_save_creds(const char *ssid, const char *pass)
{
    nvs_handle_t nvs;
    ESP_ERROR_CHECK(nvs_open("wifi", NVS_READWRITE, &nvs));
    ESP_ERROR_CHECK(nvs_set_str(nvs, "ssid", ssid));
    ESP_ERROR_CHECK(nvs_set_str(nvs, "pass", pass));
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);
}

void portal_clear_creds(void)
{
    nvs_handle_t nvs;
    if (nvs_open("wifi", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_erase_all(nvs);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
}

// --- The liar's DNS ------------------------------------------------------------
// A minimal DNS responder: take any query, echo it back as a response
// whose single answer is our IP with a short TTL. No parsing beyond the
// header — the question section is copied verbatim.
static void dns_task(void *arg)
{
    (void)arg;
    const int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(53),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));

    uint8_t buf[512];
    while (1) {
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);
        const int len = recvfrom(sock, buf, sizeof(buf) - 16, 0,
                                 (struct sockaddr *)&from, &from_len);
        if (len < 12) continue;
        buf[2] = 0x81; buf[3] = 0x80; // response, recursion available
        buf[6] = 0; buf[7] = 1;       // one answer
        buf[8] = buf[9] = buf[10] = buf[11] = 0;
        int p = len;
        buf[p++] = 0xC0; buf[p++] = 12; // pointer to the question name
        buf[p++] = 0; buf[p++] = 1;     // type A
        buf[p++] = 0; buf[p++] = 1;     // class IN
        buf[p++] = 0; buf[p++] = 0; buf[p++] = 0; buf[p++] = 30; // TTL
        buf[p++] = 0; buf[p++] = 4;     // rdlength
        buf[p++] = 192; buf[p++] = 168; buf[p++] = 4; buf[p++] = 1;
        sendto(sock, buf, p, 0, (struct sockaddr *)&from, from_len);
    }
}

// --- HTTP ------------------------------------------------------------------------
static portal_submit_cb_t submit_cb;

static esp_err_t root_get(httpd_req_t *req)
{
    // Scan from the portal page load, so the list is fresh.
    wifi_scan_config_t scan = {0};
    esp_wifi_scan_start(&scan, true);
    uint16_t n = 12;
    wifi_ap_record_t aps[12];
    esp_wifi_scan_get_ap_records(&n, aps);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr_chunk(
        req,
        "<!doctype html><meta name=viewport content='width=device-width,"
        "initial-scale=1'><title>ESPtember setup</title>"
        "<body style='font-family:sans-serif;background:#111;color:#eee;"
        "padding:24px'><h2 style='color:#ff5b04'>ESPtember setup</h2>"
        "<form method=post action=/save>"
        "<label>Network<br><select name=ssid style='width:100%;padding:8px;"
        "font-size:16px'>");
    char option[80];
    for (int i = 0; i < n; i++) {
        snprintf(option, sizeof(option), "<option>%.32s</option>",
                 (const char *)aps[i].ssid);
        httpd_resp_sendstr_chunk(req, option);
    }
    httpd_resp_sendstr_chunk(
        req,
        "</select></label><br><br><label>Password<br>"
        "<input name=pass type=password style='width:100%;padding:8px;"
        "font-size:16px'></label><br><br>"
        "<button style='padding:10px 24px;font-size:16px;background:#ff5b04;"
        "border:0;color:#000'>Save & connect</button></form>");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static void url_decode(char *s)
{
    char *o = s;
    while (*s) {
        if (*s == '+') *o++ = ' ';
        else if (*s == '%' && s[1] && s[2]) {
            char hex[3] = {s[1], s[2], 0};
            *o++ = (char)strtol(hex, NULL, 16);
            s += 2;
        } else *o++ = *s;
        s++;
    }
    *o = 0;
}

static esp_err_t save_post(httpd_req_t *req)
{
    char body[192] = {0};
    const int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0) return ESP_FAIL;
    char ssid[33] = {0}, pass[65] = {0};
    httpd_query_key_value(body, "ssid", ssid, sizeof(ssid));
    httpd_query_key_value(body, "pass", pass, sizeof(pass));
    url_decode(ssid);
    url_decode(pass);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req,
                       "<body style='font-family:sans-serif;background:#111;"
                       "color:#eee;padding:24px'><h2 style='color:#ff5b04'>"
                       "Saved.</h2>The board is rebooting onto your network.");
    if (submit_cb) submit_cb(ssid, pass);
    return ESP_OK;
}

// Any unknown URL (the phone's connectivity probes) redirects to root —
// the second half of the captive-portal handshake.
static esp_err_t redirect_handler(httpd_req_t *req, httpd_err_code_t err)
{
    (void)err;
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://" AP_IP "/");
    httpd_resp_sendstr(req, "");
    return ESP_OK;
}

void portal_start(portal_submit_cb_t cb)
{
    submit_cb = cb;

    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    wifi_config_t ap = {
        .ap = {.ssid = AP_SSID,
               .max_connection = 4,
               .authmode = WIFI_AUTH_OPEN},
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA)); // STA for scanning
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    ESP_ERROR_CHECK(esp_wifi_start());

    xTaskCreate(dns_task, "dns", 4096, NULL, 5, NULL);

    httpd_handle_t server;
    httpd_config_t http_cfg = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&server, &http_cfg));
    const httpd_uri_t root = {.uri = "/", .method = HTTP_GET,
                              .handler = root_get};
    const httpd_uri_t save = {.uri = "/save", .method = HTTP_POST,
                              .handler = save_post};
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &save);
    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, redirect_handler);
    printf("D21_PORTAL up ssid=%s ip=%s\n", AP_SSID, AP_IP);
}
