// ESPtember Day 24 — Internet Walkie-Talkie, transmitter half (StopWatch)
// Day 23 with one organ transplanted: the transport. The mic, the PTT
// crown, the 15 ms frames, the channel dial — identical. But instead of
// ESP-NOW broadcast, frames ride a WebSocket to a Cloudflare Durable
// Object that fans them out to everyone on the channel, anywhere on
// Earth. Same UI, new radio: the transport-swap lesson.
//
// Wi-Fi credentials come from day 22's NVS namespace (`wifi SSID PASS`
// over serial, once, shared across lessons).
#include <M5Unified.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <Preferences.h>
#include "wifi_portal.h"

#define RELAY_HOST "esptember-walkie-relay.chantastic.workers.dev"
#define SAMPLE_RATE 8000
#define FRAME_SAMPLES 120

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint16_t seq;
  uint8_t channel;
  uint8_t flags;
  int16_t pcm[FRAME_SAMPLES];
} wt_frame_t;

#define WT_MAGIC 0x30574B54u

static Preferences prefs;
static WebSocketsClient ws;
static uint8_t channel = 1;
static uint16_t seq = 0;
static bool talking = false;
static bool wsConnected = false;
static uint32_t framesSent = 0;
static int16_t mic[FRAME_SAMPLES];

static void wsConnect() {
  char path[24];
  snprintf(path, sizeof(path), "/channel/%u", channel);
  ws.disconnect();
  ws.beginSSL(RELAY_HOST, 443, path);
  ws.setReconnectInterval(2000);
}

static void onWsEvent(WStype_t type, uint8_t *payload, size_t len) {
  (void)payload; (void)len;
  if (type == WStype_CONNECTED) {
    wsConnected = true;
    Serial.println("D24_WS connected");
  } else if (type == WStype_DISCONNECTED) {
    wsConnected = false;
  }
}

static void sendFrame(const int16_t *pcm, bool start) {
  wt_frame_t frame;
  frame.magic = WT_MAGIC;
  frame.seq = seq++;
  frame.channel = channel;
  frame.flags = start ? 1 : 0;
  memcpy(frame.pcm, pcm, sizeof(frame.pcm));
  ws.sendBIN((uint8_t *)&frame, sizeof(frame));
  framesSent++;
}

// --- Display -------------------------------------------------------------
static M5Canvas canvas(&M5.Display);

static void render() {
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(0xFD20, TFT_BLACK);
  canvas.setTextSize(2);
  canvas.drawString("WALKIE  NET TX", 233, 70);

  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setTextSize(7);
  char big[8];
  snprintf(big, sizeof(big), "%u", channel);
  canvas.drawString(big, 233, 180);
  canvas.setTextSize(2);
  canvas.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  canvas.drawString("channel", 233, 240);

  const char *net = WiFi.status() != WL_CONNECTED ? "no Wi-Fi"
                    : wsConnected                 ? "relay linked"
                                                  : "linking...";
  canvas.setTextColor(wsConnected ? 0x07E0 : TFT_DARKGRAY, TFT_BLACK);
  canvas.drawString(net, 233, 290);

  if (talking) {
    canvas.fillCircle(233, 340, 20, 0xF800);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("ON AIR", 233, 385);
  } else {
    canvas.drawCircle(233, 340, 20, TFT_DARKGRAY);
    canvas.setTextColor(TFT_DARKGRAY, TFT_BLACK);
    canvas.drawString("hold A to talk", 233, 385);
  }
  canvas.drawString("B: channel up", 233, 420);
  canvas.pushSprite(0, 0);
}

static void reportStatus() {
  Serial.printf("D24_STATUS ch=%u wifi=%d ws=%d talking=%d sent=%lu\n",
                channel, WiFi.status() == WL_CONNECTED, (int)wsConnected,
                (int)talking, (unsigned long)framesSent);
}

static void handleSerial() {
  static char line[128];
  static int len = 0;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\n' || c == '\r') {
      line[len] = 0;
      len = 0;
      char ssid[64], pass[64];
      if (sscanf(line, "wifi %63s %63s", ssid, pass) == 2) {
        prefs.putString("ssid", ssid);
        prefs.putString("pass", pass);
        Serial.printf("D24_SAVED ssid=%s\n", ssid);
        ESP.restart();
      } else if (!strcmp(line, "t")) {
        Serial.println("D24_TESTTONE start");
        int16_t tone[FRAME_SAMPLES];
        float phase = 0;
        for (int f = 0; f < 66; f++) {
          for (int i = 0; i < FRAME_SAMPLES; i++) {
            phase += 2.0f * (float)M_PI * 440.0f / SAMPLE_RATE;
            tone[i] = (int16_t)(sinf(phase) * 12000);
          }
          sendFrame(tone, f == 0);
          ws.loop();
          delay(15);
        }
        Serial.println("D24_TESTTONE end");
        reportStatus();
      } else if (!strcmp(line, "s")) {
        reportStatus();
      }
    } else if (len + 1 < (int)sizeof(line)) line[len++] = c;
  }
}

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = true;
  cfg.internal_mic = true;
  cfg.internal_spk = false;
  cfg.fallback_board = m5::board_t::board_M5StopWatch;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(255);
  canvas.setColorDepth(8);
  canvas.createSprite(466, 466);
  M5.Mic.begin();

  prefs.begin("day22", false); // deliberately day 22's namespace:
                               // provision once, every lesson benefits
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
  ws.onEvent(onWsEvent);
  render();
  Serial.println("D24_READY tx");
}

static bool wsStarted = false;
static uint32_t lastRender = 0;

void loop() {
  M5.update();
  if (WiFi.status() == WL_CONNECTED && !wsStarted) {
    wsStarted = true;
    wsConnect();
  }
  ws.loop();

  const bool ptt = M5.BtnA.isPressed();
  if (ptt != talking) {
    talking = ptt;
    render();
    reportStatus();
  }
  if (talking && wsConnected) {
    if (M5.Mic.record(mic, FRAME_SAMPLES, SAMPLE_RATE)) {
      static bool wasTalking = false;
      sendFrame(mic, !wasTalking);
      wasTalking = talking;
    }
  }

  if (M5.BtnB.wasClicked()) {
    channel = channel % 22 + 1;
    if (wsConnected) wsConnect(); // rooms are URLs: rejoin the new one
    render();
    reportStatus();
  }

  handleSerial();
  if (millis() - lastRender > 1000) {
    lastRender = millis();
    render();
  }
  delay(1);
}
