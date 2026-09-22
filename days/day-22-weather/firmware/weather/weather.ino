// ESPtember Day 22 — Weather Station
// The portal's payoff: the board is online and the sky is the first
// thing worth fetching. Open-Meteo serves current conditions and
// forecasts with no API key, no account, no strings — pure JSON over
// HTTPS. The round face becomes a watch complication: big temperature,
// condition, high/low, wind, and a 12-hour temperature arc around the
// rim.
//
// Provisioning on this board is serial (the StopWatch can't show the
// day-21 portal - no keyboard, no browser): send `wifi SSID PASS` once;
// creds persist in NVS. `loc LAT LON` sets the place (default:
// Portland, OR). `forget` clears.
#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "wifi_portal.h"

static Preferences prefs;

// --- Weather state -------------------------------------------------------------
static float temperature = NAN, windKph = NAN, tempHigh = NAN, tempLow = NAN;
static int weatherCode = -1;
static float hourly[12];
static int hourlyCount = 0;
static uint32_t lastFetch = 0;
static char statusLine[48] = "starting";

// WMO weather interpretation codes, the ones Open-Meteo returns.
static const char *describe(int code) {
  if (code == 0) return "CLEAR";
  if (code <= 2) return "PARTLY CLOUDY";
  if (code == 3) return "OVERCAST";
  if (code <= 48) return "FOG";
  if (code <= 57) return "DRIZZLE";
  if (code <= 67) return "RAIN";
  if (code <= 77) return "SNOW";
  if (code <= 82) return "SHOWERS";
  if (code <= 86) return "SNOW SHOWERS";
  return "THUNDERSTORM";
}

static bool fetchWeather() {
  const float lat = prefs.getFloat("lat", 45.52f);
  const float lon = prefs.getFloat("lon", -122.68f);
  char url[256];
  snprintf(url, sizeof(url),
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude="
           "%.4f&current=temperature_2m,weather_code,wind_speed_10m"
           "&hourly=temperature_2m&daily=temperature_2m_max,temperature_2m_min"
           "&forecast_hours=12&forecast_days=1&timezone=auto",
           lat, lon);
  HTTPClient http;
  http.begin(url); // Open-Meteo's cert chain is public CA; esp32 core
                   // bundles the roots when available, else falls back
  http.setTimeout(10000);
  const int code = http.GET();
  if (code != 200) {
    snprintf(statusLine, sizeof(statusLine), "HTTP %d", code);
    http.end();
    return false;
  }
  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    snprintf(statusLine, sizeof(statusLine), "bad JSON");
    return false;
  }
  temperature = doc["current"]["temperature_2m"] | NAN;
  weatherCode = doc["current"]["weather_code"] | -1;
  windKph = doc["current"]["wind_speed_10m"] | NAN;
  tempHigh = doc["daily"]["temperature_2m_max"][0] | NAN;
  tempLow = doc["daily"]["temperature_2m_min"][0] | NAN;
  hourlyCount = 0;
  for (JsonVariant v : doc["hourly"]["temperature_2m"].as<JsonArray>()) {
    if (hourlyCount >= 12) break;
    hourly[hourlyCount++] = v.as<float>();
  }
  snprintf(statusLine, sizeof(statusLine), "ok");
  Serial.printf("D22_WEATHER temp=%.1f code=%d wind=%.1f hi=%.1f lo=%.1f "
                "hours=%d\n",
                (double)temperature, weatherCode, (double)windKph,
                (double)tempHigh, (double)tempLow, hourlyCount);
  return true;
}

// --- Display ---------------------------------------------------------------------
static M5Canvas canvas(&M5.Display);

#define CX 233
#define CY 233

static void render() {
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextDatum(middle_center);

  if (WiFi.status() != WL_CONNECTED) {
    canvas.setTextColor(0xFD20, TFT_BLACK);
    canvas.setTextSize(3);
    canvas.drawString("WEATHER", CX, 150);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("no Wi-Fi", CX, 220);
    canvas.drawString("serial: wifi SSID PASS", CX, 260);
    canvas.pushSprite(0, 0);
    return;
  }

  // 12-hour temperature arc around the rim: min..max maps to the
  // bottom..top of a 270-degree sweep.
  if (hourlyCount >= 2) {
    float lo = hourly[0], hi = hourly[0];
    for (int i = 1; i < hourlyCount; i++) {
      if (hourly[i] < lo) lo = hourly[i];
      if (hourly[i] > hi) hi = hourly[i];
    }
    const float span = (hi - lo) < 1.0f ? 1.0f : hi - lo;
    for (int i = 0; i < hourlyCount; i++) {
      const float frac = (hourly[i] - lo) / span;
      const float ang =
          (135.0f + 270.0f * i / (hourlyCount - 1)) * (float)M_PI / 180.0f;
      const int r = 190 + (int)(28 * frac);
      const int x = CX + (int)(cosf(ang) * r);
      const int y = CY + (int)(sinf(ang) * r);
      canvas.fillCircle(x, y, 5, canvas.color565(255, 91 + (int)(80 * frac), 4));
    }
  }

  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setTextSize(7);
  char big[16];
  if (!isnan(temperature)) snprintf(big, sizeof(big), "%.0f", (double)temperature);
  else snprintf(big, sizeof(big), "--");
  canvas.drawString(big, CX, 180);
  canvas.setTextSize(3);
  canvas.setTextColor(0xFD20, TFT_BLACK);
  canvas.drawString("C", CX + 90, 160);

  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setTextSize(2);
  canvas.drawString(weatherCode >= 0 ? describe(weatherCode) : "...", CX, 250);

  char line[40];
  if (!isnan(tempHigh))
    snprintf(line, sizeof(line), "H %.0f   L %.0f", (double)tempHigh,
             (double)tempLow);
  else snprintf(line, sizeof(line), " ");
  canvas.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  canvas.drawString(line, CX, 300);
  if (!isnan(windKph)) {
    snprintf(line, sizeof(line), "wind %.0f km/h", (double)windKph);
    canvas.drawString(line, CX, 336);
  }
  canvas.pushSprite(0, 0);
}

// --- Serial provisioning ------------------------------------------------------
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
        Serial.printf("D22_SAVED ssid=%s\n", ssid);
        ESP.restart();
      } else if (sscanf(line, "loc %f %f", &lat, &lon) == 2) {
        prefs.putFloat("lat", lat);
        prefs.putFloat("lon", lon);
        Serial.printf("D22_LOC %.4f %.4f\n", (double)lat, (double)lon);
        lastFetch = 0; // refetch now
      } else if (!strcmp(line, "forget")) {
        prefs.clear();
        Serial.println("D22_FORGOT");
        ESP.restart();
      } else if (!strcmp(line, "s")) {
        Serial.printf("D22_STATUS wifi=%d status=%s temp=%.1f\n",
                      WiFi.status() == WL_CONNECTED, statusLine,
                      (double)temperature);
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

  prefs.begin("day22", false);
  const String ssid = prefs.getString("ssid", "");
  if (!ssid.length()) {
    // No Wi-Fi: become the setup page (shared portal, day 21's pattern).
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
  snprintf(statusLine, sizeof(statusLine), "joining %s", ssid.c_str());
  render();
  Serial.println("D22_READY");
}

void loop() {
  M5.update();
  const uint32_t now = millis();

  // Fetch on connect, then every 10 minutes. B forces a refresh.
  const bool due = WiFi.status() == WL_CONNECTED &&
                   (!lastFetch || now - lastFetch >= 600000 ||
                    M5.BtnB.wasClicked());
  if (due) {
    lastFetch = now;
    fetchWeather();
  }


  // Hold both pushers ~2 s: forget Wi-Fi, reopen the setup portal.
  static uint32_t chordSince = 0;
  if (M5.BtnA.isPressed() && M5.BtnB.isPressed()) {
    if (!chordSince) chordSince = millis();
    else if (millis() - chordSince > 2000) {
      prefs.remove("ssid");
      prefs.remove("pass");
      Serial.println("WIFI_RESET");
      ESP.restart();
    }
  } else {
    chordSince = 0;
  }

  handleSerial();

  static uint32_t lastDraw = 0;
  if (now - lastDraw >= 1000) {
    lastDraw = now;
    render();
  }
  delay(10);
}
