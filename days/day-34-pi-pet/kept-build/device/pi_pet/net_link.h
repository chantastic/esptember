#pragma once
// Wi-Fi link to the pi-pet hub (disposable candidate).
//
// USB is the trusted provisioning channel: the hub pushes Wi-Fi credentials (shared
// ESPtember "day22" namespace) and a pairing token ("pi-pet" namespace).
// A background task joins Wi-Fi, advertises pi-pet-<id4>.local / _pi-pet._tcp, and listens.
// The hub dials in (outgoing connections pass the Mac's application firewall without a
// prompt) and both sides run a mutual HMAC-SHA256 handshake. Once the link is up, the main
// loop reads protocol lines from it and GB_* output is mirrored to it.
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>

static void processLine(char *line, bool fromUsb);  // defined in the sketch

namespace netlink {

static char deviceId[13];
static char token[65];          // 64 hex chars (32 bytes); empty = unpaired
static char hubHost[48], hubIp[16];
static uint16_t hubPort = 47837;
static char wifiSsid[33], wifiPass[65];
static WiFiClient client;
static WiFiServer server(47837);
static SemaphoreHandle_t clientMutex;  // guards `client` between the net task and the main loop
static volatile bool linkUp = false;      // handshake complete; main loop owns `client`
static volatile bool wantReconnect = false;
static volatile uint32_t linkEvents = 0;  // successful handshakes (evidence)
static char lastError[24] = "none";
static SemaphoreHandle_t cfgMutex;

static void hexOf(const uint8_t *b, size_t n, char *out) {
  static const char *h = "0123456789abcdef";
  for (size_t i = 0; i < n; ++i) { out[2 * i] = h[b[i] >> 4]; out[2 * i + 1] = h[b[i] & 15]; }
  out[2 * n] = 0;
}

static void hmacHex(const char *key, const char *msg, char *out65) {
  uint8_t mac[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, reinterpret_cast<const uint8_t *>(key), strlen(key));
  mbedtls_md_hmac_update(&ctx, reinterpret_cast<const uint8_t *>(msg), strlen(msg));
  mbedtls_md_hmac_finish(&ctx, mac);
  mbedtls_md_free(&ctx);
  hexOf(mac, 32, out65);
}

// First 8 hex chars of SHA-256(token): lets the hub check a pairing without seeing it again.
static void keyPrint(char *out9) {
  if (!token[0]) { strcpy(out9, "-"); return; }
  uint8_t d[32];
  mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), reinterpret_cast<const uint8_t *>(token), strlen(token), d);
  char full[65];
  hexOf(d, 32, full);
  memcpy(out9, full, 8); out9[8] = 0;
}

static void load() {
  uint64_t mac = ESP.getEfuseMac();
  uint8_t b[6];
  for (int i = 0; i < 6; ++i) b[i] = uint8_t(mac >> (8 * i));
  hexOf(b, 6, deviceId);
  Preferences p;
  if (p.begin("day22", true)) {  // shared ESPtember Wi-Fi home
    strncpy(wifiSsid, p.getString("ssid", "").c_str(), sizeof(wifiSsid) - 1);
    strncpy(wifiPass, p.getString("pass", "").c_str(), sizeof(wifiPass) - 1);
    p.end();
  }
  if (p.begin("pi-pet", true)) {
    strncpy(token, p.getString("token", "").c_str(), sizeof(token) - 1);
    strncpy(hubHost, p.getString("host", "").c_str(), sizeof(hubHost) - 1);
    strncpy(hubIp, p.getString("ip", "").c_str(), sizeof(hubIp) - 1);
    hubPort = p.getUShort("port", 47837);
    p.end();
  }
}

static const char *wifiState() {
  if (!wifiSsid[0]) return "none";
  return WiFi.status() == WL_CONNECTED ? "connected" : "saved";
}

static void hello(Print &out) {
  char kp[9];
  keyPrint(kp);
  out.printf("GB_HELLO id=%s paired=%d key=%s wifi=%s ip=%s hub=%s:%u link=%s err=%s battery=%d charging=%d\n", deviceId,
             token[0] != 0, kp, wifiState(), WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "-",
             hubIp[0] ? hubIp : "-", hubPort, linkUp ? "wifi" : "-", lastError, int(M5.Power.getBatteryLevel()),
             int(M5.Power.isCharging()));
}

static bool b64decode(const char *in, char *out, size_t cap) {
  size_t olen = 0;
  if (mbedtls_base64_decode(reinterpret_cast<uint8_t *>(out), cap - 1, &olen,
                            reinterpret_cast<const uint8_t *>(in), strlen(in)) != 0) return false;
  out[olen] = 0;
  return true;
}

static void dropLink() {
  if (!linkUp) return;
  xSemaphoreTake(clientMutex, portMAX_DELAY);
  linkUp = false;
  client.stop();
  xSemaphoreGive(clientMutex);
}

// Provisioning commands: accepted only from USB (the trusted channel). Returns true if handled.
static bool provision(char *line, Print &out) {
  if (!strcmp(line, "hello")) { hello(out); return true; }
  if (!strncmp(line, "wifi ", 5)) {  // wifi <base64 ssid> <base64 password>  (never echoed)
    char *a = strtok(line + 5, " "), *b = strtok(nullptr, " ");
    char s[33] = "", pw[65] = "";
    if (!a || !b64decode(a, s, sizeof(s)) || (b && !b64decode(b, pw, sizeof(pw))) || !s[0]) {
      out.println("GB_ERROR bad_wifi"); return true;
    }
    Preferences p;
    if (p.begin("day22", false)) { p.putString("ssid", s); p.putString("pass", pw); p.end(); }
    xSemaphoreTake(cfgMutex, portMAX_DELAY);
    strcpy(wifiSsid, s); strcpy(wifiPass, pw);
    xSemaphoreGive(cfgMutex);
    WiFi.disconnect(false, false);
    dropLink();
    out.printf("GB_WIFI saved ssid_len=%u\n", unsigned(strlen(s)));
    return true;
  }
  if (!strncmp(line, "pair ", 5)) {  // pair <64-hex token> <host> <ip> <port>
    char *t = strtok(line + 5, " "), *h = strtok(nullptr, " "), *ip = strtok(nullptr, " "), *pt = strtok(nullptr, " ");
    if (!t || strlen(t) != 64 || !h || !ip || !pt) { out.println("GB_ERROR bad_pair"); return true; }
    Preferences p;
    if (p.begin("pi-pet", false)) {
      p.putString("token", t); p.putString("host", h); p.putString("ip", ip); p.putUShort("port", uint16_t(atoi(pt)));
      p.end();
    }
    xSemaphoreTake(cfgMutex, portMAX_DELAY);
    strcpy(token, t); strncpy(hubHost, h, sizeof(hubHost) - 1); strncpy(hubIp, ip, sizeof(hubIp) - 1);
    hubPort = uint16_t(atoi(pt));
    xSemaphoreGive(cfgMutex);
    dropLink();
    char kp[9]; keyPrint(kp);
    out.printf("GB_PAIRED key=%s\n", kp);
    return true;
  }
  if (!strncmp(line, "hubaddr ", 8)) {  // hubaddr <host> <ip> <port>: keep the address current
    char *h = strtok(line + 8, " "), *ip = strtok(nullptr, " "), *pt = strtok(nullptr, " ");
    if (!h || !ip || !pt) return true;
    if (strcmp(ip, hubIp) || uint16_t(atoi(pt)) != hubPort) {
      Preferences p;
      if (p.begin("pi-pet", false)) { p.putString("host", h); p.putString("ip", ip); p.putUShort("port", uint16_t(atoi(pt))); p.end(); }
      xSemaphoreTake(cfgMutex, portMAX_DELAY);
      strncpy(hubHost, h, sizeof(hubHost) - 1); strncpy(hubIp, ip, sizeof(hubIp) - 1); hubPort = uint16_t(atoi(pt));
      xSemaphoreGive(cfgMutex);
    }
    return true;
  }
  if (!strcmp(line, "forget")) {  // forget the pairing (Wi-Fi credentials are shared; kept)
    Preferences p;
    if (p.begin("pi-pet", false)) { p.clear(); p.end(); }
    xSemaphoreTake(cfgMutex, portMAX_DELAY);
    token[0] = hubHost[0] = hubIp[0] = 0;
    xSemaphoreGive(cfgMutex);
    dropLink();
    out.println("GB_FORGOT");
    return true;
  }
  return false;
}

static bool readLine(WiFiClient &client, char *buf, size_t cap, uint32_t timeoutMs) {
  size_t n = 0;
  const uint32_t t0 = millis();
  while (millis() - t0 < timeoutMs) {
    while (client.available()) {
      const char c = client.read();
      if (c == '\n') { buf[n] = 0; return true; }
      if (c != '\r' && n < cap - 1) buf[n++] = c;
    }
    if (!client.connected()) return false;
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  return false;
}

static void setError(const char *e) { strncpy(lastError, e, sizeof(lastError) - 1); }

// Authenticate a hub that dialed in: HELLO id nonceD -> CHALLENGE nonceH macH -> AUTH macD -> OK.
static bool handshake(WiFiClient &client) {
  client.setNoDelay(true);
  uint8_t nd[8];
  esp_fill_random(nd, sizeof(nd));
  char nonceD[17];
  hexOf(nd, 8, nonceD);
  client.printf("HELLO %s %s\n", deviceId, nonceD);
  char line[200];
  if (!readLine(client, line, sizeof(line), 3000)) { setError("no_challenge"); client.stop(); return false; }
  char nonceH[33], macH[65];
  if (sscanf(line, "CHALLENGE %32s %64s", nonceH, macH) != 2) { setError(!strncmp(line, "DENY", 4) ? "denied" : "bad_challenge"); client.stop(); return false; }
  char msg[80], expect[65];
  snprintf(msg, sizeof(msg), "hub:%s:%s", nonceD, nonceH);
  hmacHex(token, msg, expect);
  if (strcmp(expect, macH)) { setError("hub_auth"); client.stop(); return false; }  // not our hub
  snprintf(msg, sizeof(msg), "dev:%s:%s", nonceH, nonceD);
  char macD[65];
  hmacHex(token, msg, macD);
  client.printf("AUTH %s\n", macD);
  if (!readLine(client, line, sizeof(line), 3000) || strcmp(line, "OK")) { setError("dev_auth"); client.stop(); return false; }
  setError("none");
  return true;
}

static void task(void *) {
  bool mdnsUp = false, serverUp = false;
  uint32_t lastWifiBegin = 0;
  char ssid[33] = "";
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(100));
    xSemaphoreTake(cfgMutex, portMAX_DELAY);
    const bool haveWifi = wifiSsid[0] != 0, paired = token[0] != 0;
    const bool ssidChanged = strcmp(ssid, wifiSsid) != 0;
    if (ssidChanged) strcpy(ssid, wifiSsid);
    char pass[65]; strcpy(pass, wifiPass);
    xSemaphoreGive(cfgMutex);
    if (!haveWifi) continue;
    if (WiFi.status() != WL_CONNECTED) {
      if (ssidChanged || millis() - lastWifiBegin > 15000) {
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(true);  // modem sleep between beacons
        WiFi.begin(ssid, pass);
        lastWifiBegin = millis();
      }
      continue;
    }
    if (!mdnsUp) {
      char name[24];
      snprintf(name, sizeof(name), "pi-pet-%s", deviceId + 8);
      mdnsUp = MDNS.begin(name);
      if (mdnsUp) MDNS.addService("pi-pet", "tcp", 47837);
    }
    if (!serverUp) { server.begin(); server.setNoDelay(true); serverUp = true; }
    WiFiClient incoming = server.accept();
    if (!incoming) continue;
    if (!paired) { incoming.print("DENY unpaired\n"); incoming.stop(); continue; }
    if (!handshake(incoming)) continue;
    // A newly authenticated hub replaces any previous link (e.g. after the hub restarted).
    xSemaphoreTake(clientMutex, portMAX_DELAY);
    if (linkUp) client.stop();
    client = incoming;
    linkUp = true;
    ++linkEvents;
    xSemaphoreGive(clientMutex);
  }
}

static void begin() {
  cfgMutex = xSemaphoreCreateMutex();
  clientMutex = xSemaphoreCreateMutex();
  load();
  xTaskCreatePinnedToCore(task, "netlink", 6144, nullptr, 1, nullptr, 0);
}

// Main loop: read protocol lines from the hub while the link is up.
static void poll() {
  if (!linkUp) return;
  static char line[200];
  static size_t n = 0;
  static char pending[4][200];
  int np = 0;
  xSemaphoreTake(clientMutex, portMAX_DELAY);
  if (!client.connected()) { linkUp = false; client.stop(); setError("dropped"); }
  else {
    while (client.available() && np < 4) {
      const char c = client.read();
      if (c == '\n') { line[n] = 0; n = 0; if (line[0]) strcpy(pending[np++], line); }
      else if (c != '\r' && n < sizeof(line) - 1) line[n++] = c;
    }
  }
  xSemaphoreGive(clientMutex);
  for (int i = 0; i < np; ++i) processLine(pending[i], false);  // outside the lock: may print
}

// GB_* output goes to USB and, while the link is up, to the hub (buffered per line).
class TeeOut : public Print {
 public:
  size_t write(uint8_t c) override {
    Serial.write(c);
    if (n_ < sizeof(buf_)) buf_[n_++] = char(c);
    if (c == '\n') {
      if (linkUp && xSemaphoreTake(clientMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        if (linkUp && client.connected()) client.write(reinterpret_cast<const uint8_t *>(buf_), n_);
        xSemaphoreGive(clientMutex);
      }
      n_ = 0;
    }
    return 1;
  }
 private:
  char buf_[256];
  size_t n_ = 0;
};

}  // namespace netlink

static netlink::TeeOut netOut;
