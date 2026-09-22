// ESPtember Day 25 — Flight Radar
// The round face becomes a radar scope: you at the center, every
// aircraft within 25 nautical miles as a blip with callsign and
// altitude, fed by adsb.lol — a community aggregator of hobbyist ADS-B
// receivers, free and keyless. Planes broadcast their position
// unencrypted at 1090 MHz; volunteers with $20 dongles catch it; this
// board just asks nicely every eight seconds.
//
// Provisioning: day 22's serial channel and NVS namespace (`wifi SSID
// PASS`, `loc LAT LON`).
#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "wifi_portal.h"

#define RADIUS_NM 25
#define FETCH_MS 8000

static Preferences prefs;

// --- Aircraft table -----------------------------------------------------------
typedef struct {
  char callsign[10];
  float lat, lon;
  int altitude;   // feet
  int track;      // degrees
  float distance; // nm from center
} aircraft_t;

#define MAX_AIRCRAFT 16
static aircraft_t aircraft[MAX_AIRCRAFT];
static int aircraftCount = 0;
static uint32_t lastFetch = 0;
static uint32_t fetches = 0;
static char statusLine[32] = "starting";

static bool fetchAircraft() {
  const float lat = prefs.getFloat("lat", 45.52f);
  const float lon = prefs.getFloat("lon", -122.68f);
  char url[128];
  snprintf(url, sizeof(url), "https://api.adsb.lol/v2/point/%.4f/%.4f/%d",
           lat, lon, RADIUS_NM);
  HTTPClient http;
  http.begin(url);
  http.setTimeout(8000);
  const int code = http.GET();
  if (code != 200) {
    snprintf(statusLine, sizeof(statusLine), "HTTP %d", code);
    http.end();
    return false;
  }
  // Filter the parse: only the fields the scope draws. A full airliner
  // record is large; the filter keeps the JSON work tiny.
  JsonDocument filter;
  filter["ac"][0]["flight"] = true;
  filter["ac"][0]["lat"] = true;
  filter["ac"][0]["lon"] = true;
  filter["ac"][0]["alt_baro"] = true;
  filter["ac"][0]["track"] = true;
  JsonDocument doc;
  const DeserializationError err = deserializeJson(
      doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) {
    snprintf(statusLine, sizeof(statusLine), "bad JSON");
    return false;
  }
  aircraftCount = 0;
  for (JsonObject ac : doc["ac"].as<JsonArray>()) {
    if (aircraftCount >= MAX_AIRCRAFT) break;
    if (!ac["lat"].is<float>()) continue;
    aircraft_t *a = &aircraft[aircraftCount];
    const char *flight = ac["flight"] | "";
    snprintf(a->callsign, sizeof(a->callsign), "%s", flight);
    // Trim the padding ADS-B callsigns carry.
    for (int i = strlen(a->callsign) - 1; i >= 0 && a->callsign[i] == ' '; i--)
      a->callsign[i] = 0;
    a->lat = ac["lat"];
    a->lon = ac["lon"];
    a->altitude = ac["alt_baro"] | 0;
    a->track = ac["track"] | 0;
    // Equirectangular is plenty at 25 nm.
    const float dLat = (a->lat - lat) * 60.0f;
    const float dLon = (a->lon - lon) * 60.0f * cosf(lat * M_PI / 180.0f);
    a->distance = sqrtf(dLat * dLat + dLon * dLon);
    aircraftCount++;
  }
  fetches++;
  snprintf(statusLine, sizeof(statusLine), "ok");
  Serial.printf("D25_AIRCRAFT n=%d fetch=%lu\n", aircraftCount,
                (unsigned long)fetches);
  return true;
}

// --- Radar scope ---------------------------------------------------------------
static M5Canvas canvas(&M5.Display);

#define CX 233
#define CY 233
#define SCOPE_R 210

static void render() {
  const uint32_t now = millis();
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextDatum(middle_center);

  if (WiFi.status() != WL_CONNECTED) {
    canvas.setTextColor(0xFD20, TFT_BLACK);
    canvas.setTextSize(3);
    canvas.drawString("RADAR", CX, 160);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("no Wi-Fi", CX, 230);
    canvas.drawString("serial: wifi SSID PASS", CX, 270);
    canvas.pushSprite(0, 0);
    return;
  }

  // Range rings at 5nm spacing, dim green like the real thing.
  const uint16_t ring = canvas.color565(0, 80, 0);
  for (int nm = 5; nm <= RADIUS_NM; nm += 10)
    canvas.drawCircle(CX, CY, SCOPE_R * nm / RADIUS_NM, ring);
  canvas.drawCircle(CX, CY, SCOPE_R, canvas.color565(0, 130, 0));

  // The sweep: one revolution per fetch interval, purely cosmetic,
  // entirely mandatory.
  const float sweepAng = 2.0f * (float)M_PI * (now % FETCH_MS) / FETCH_MS -
                         (float)M_PI / 2;
  canvas.drawLine(CX, CY, CX + (int)(cosf(sweepAng) * SCOPE_R),
                  CY + (int)(sinf(sweepAng) * SCOPE_R),
                  canvas.color565(0, 200, 0));

  canvas.fillCircle(CX, CY, 5, TFT_WHITE); // you are here

  const float lat = prefs.getFloat("lat", 45.52f);
  const float lon = prefs.getFloat("lon", -122.68f);
  for (int i = 0; i < aircraftCount; i++) {
    const aircraft_t *a = &aircraft[i];
    const float dLat = (a->lat - lat) * 60.0f;
    const float dLon = (a->lon - lon) * 60.0f * cosf(lat * M_PI / 180.0f);
    const int x = CX + (int)(dLon * SCOPE_R / RADIUS_NM);
    const int y = CY - (int)(dLat * SCOPE_R / RADIUS_NM);
    if ((x - CX) * (x - CX) + (y - CY) * (y - CY) > SCOPE_R * SCOPE_R)
      continue;
    canvas.fillCircle(x, y, 5, 0x07E0);
    // Velocity leader: a short line along the track.
    const float t = (a->track - 90) * (float)M_PI / 180.0f;
    canvas.drawLine(x, y, x + (int)(cosf(t) * 14), y + (int)(sinf(t) * 14),
                    0x07E0);
    canvas.setTextSize(1);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    if (a->callsign[0]) canvas.drawString(a->callsign, x, y - 14);
    char alt[12];
    snprintf(alt, sizeof(alt), "%d", a->altitude / 100); // flight level
    canvas.setTextColor(TFT_DARKGRAY, TFT_BLACK);
    canvas.drawString(alt, x, y + 14);
  }

  canvas.setTextSize(2);
  canvas.setTextColor(0xFD20, TFT_BLACK);
  char head[24];
  snprintf(head, sizeof(head), "%d aircraft", aircraftCount);
  canvas.drawString(head, CX, 430);
  canvas.pushSprite(0, 0);
}

// --- Serial provisioning (day 22's grammar) --------------------------------------
static void handleSerial() {
  static char line[128];
  static int len = 0;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\n' || c == '\r') {
      line[len] = 0;
      len = 0;
      char ssid[64], pass[64];
      float lat, lon;
      if (sscanf(line, "wifi %63s %63s", ssid, pass) == 2) {
        prefs.putString("ssid", ssid);
        prefs.putString("pass", pass);
        Serial.printf("D25_SAVED ssid=%s\n", ssid);
        ESP.restart();
      } else if (sscanf(line, "loc %f %f", &lat, &lon) == 2) {
        prefs.putFloat("lat", lat);
        prefs.putFloat("lon", lon);
        Serial.printf("D25_LOC %.4f %.4f\n", (double)lat, (double)lon);
        lastFetch = 0;
      } else if (!strcmp(line, "s")) {
        Serial.printf("D25_STATUS wifi=%d aircraft=%d fetches=%lu status=%s\n",
                      WiFi.status() == WL_CONNECTED, aircraftCount,
                      (unsigned long)fetches, statusLine);
      }
    } else if (len + 1 < (int)sizeof(line)) line[len++] = c;
  }
}

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = true;
  cfg.internal_mic = false;
  cfg.internal_spk = false;
  cfg.fallback_board = m5::board_t::board_M5StopWatch;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(255);
  canvas.setColorDepth(8);
  canvas.createSprite(466, 466);

  prefs.begin("day22", false); // the shared provisioning namespace
  const String ssid = prefs.getString("ssid", "");
  if (!ssid.length()) {
    canvas.fillSprite(TFT_BLACK);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(0xFD20, TFT_BLACK);
    canvas.setTextSize(3);
    canvas.drawString("SETUP", 233, 150);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("join \"esptember-setup\"", 233, 220);
    canvas.pushSprite(0, 0);
    WifiPortal portal;
    portal.run(prefs, prefs, nullptr, 0); // reboots on save
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), prefs.getString("pass", "").c_str());
  render();
  Serial.println("D25_READY");
}

void loop() {
  M5.update();
  const uint32_t now = millis();
  if (WiFi.status() == WL_CONNECTED &&
      (!lastFetch || now - lastFetch >= FETCH_MS || M5.BtnB.wasClicked())) {
    lastFetch = now;
    fetchAircraft();
  }
  handleSerial();
  static uint32_t lastFrame = 0;
  if (now - lastFrame >= 50) { // the sweep wants ~20 fps
    lastFrame = now;
    render();
  }
  delay(5);
}
