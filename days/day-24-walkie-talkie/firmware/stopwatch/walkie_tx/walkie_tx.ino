// ESPtember Day 24 — Walkie-Talkie, transmitter half (StopWatch)
// Hold the crown and talk: mic audio streams over ESP-NOW to whoever is
// on your channel. 8 kHz mono 16-bit — voice quality, walkie feel.
// Each radio frame carries 120 samples (15 ms), a sequence number, and
// a channel byte: 22 logical channels ride one RF channel, exactly like
// FRS radios share the same slice of spectrum.
//
// day 21 proved the transport; today saturates it. Streams don't get
// per-frame retries — a lost 15 ms is a click, a retried 15 ms is lag —
// so frames are fire-and-forget and the receiver conceals gaps.
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define WT_MAGIC 0x30574B54u // "TKW0"
#define RF_CHANNEL 1
#define SAMPLE_RATE 8000
#define FRAME_SAMPLES 120 // 15 ms per radio frame

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint16_t seq;
  uint8_t channel; // 1..22, logical
  uint8_t flags;   // bit 0: start of transmission
  int16_t pcm[FRAME_SAMPLES];
} wt_frame_t; // 8 + 240 = 248 bytes, under ESP-NOW's 250 limit

static const uint8_t BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t channel = 1;
static uint16_t seq = 0;
static bool talking = false;
static uint32_t framesSent = 0, framesAcked = 0;
static int16_t mic[FRAME_SAMPLES];

static void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  (void)info;
  if (status == ESP_NOW_SEND_SUCCESS) framesAcked++;
}

static void sendFrame(const int16_t *pcm, bool start) {
  wt_frame_t frame;
  frame.magic = WT_MAGIC;
  frame.seq = seq++;
  frame.channel = channel;
  frame.flags = start ? 1 : 0;
  memcpy(frame.pcm, pcm, sizeof(frame.pcm));
  esp_now_send(BROADCAST, (uint8_t *)&frame, sizeof(frame));
  framesSent++;
}

// --- Display -------------------------------------------------------------
static M5Canvas canvas(&M5.Display);

static void render() {
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(0xFD20, TFT_BLACK);
  canvas.setTextSize(2);
  canvas.drawString("WALKIE  TX", 233, 70);

  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setTextSize(7);
  char big[8];
  snprintf(big, sizeof(big), "%u", channel);
  canvas.drawString(big, 233, 180);
  canvas.setTextSize(2);
  canvas.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  canvas.drawString("channel", 233, 240);

  if (talking) {
    canvas.fillCircle(233, 310, 24, 0xF800);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("ON AIR", 233, 360);
  } else {
    canvas.drawCircle(233, 310, 24, TFT_DARKGRAY);
    canvas.setTextColor(TFT_DARKGRAY, TFT_BLACK);
    canvas.drawString("hold A to talk", 233, 360);
  }
  canvas.drawString("B: channel up", 233, 400);
  canvas.pushSprite(0, 0);
}

static void reportStatus() {
  Serial.printf("D23_STATUS ch=%u talking=%d sent=%lu acked=%lu\n", channel,
                (int)talking, (unsigned long)framesSent,
                (unsigned long)framesAcked);
}

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = true;
  cfg.internal_mic = true;
  cfg.internal_spk = false; // TX half: the I2S engine is the mic's
  cfg.fallback_board = m5::board_t::board_M5StopWatch;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(255);
  canvas.setColorDepth(8);
  canvas.createSprite(466, 466);
  M5.Mic.begin();

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(RF_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_now_init();
  esp_now_register_send_cb(onSent);
  esp_now_peer_info_t bc = {};
  memcpy(bc.peer_addr, BROADCAST, 6);
  bc.channel = RF_CHANNEL;
  bc.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&bc);

  render();
  Serial.println("D23_READY tx");
}

void loop() {
  M5.update();

  const bool ptt = M5.BtnA.isPressed();
  if (ptt && !talking) {
    talking = true;
    render();
    reportStatus();
  }
  if (!ptt && talking) {
    talking = false;
    render();
    reportStatus();
  }

  if (talking) {
    // record() blocks ~15 ms filling one frame — the mic paces the
    // radio, which is exactly the cadence the receiver expects.
    if (M5.Mic.record(mic, FRAME_SAMPLES, SAMPLE_RATE)) {
      static bool wasTalking = false;
      sendFrame(mic, !wasTalking);
      wasTalking = talking;
      if (!talking) wasTalking = false;
    }
  }

  if (M5.BtnB.wasClicked()) {
    channel = channel % 22 + 1; // 1..22, wrapping
    render();
    reportStatus();
  }

  // Serial test hooks: 't' = transmit 1 s of 440 Hz test tone
  // (deterministic verification without a human voice), 's' = status.
  if (Serial.available()) {
    const char c = Serial.read();
    if (c == 't') {
      Serial.println("D23_TESTTONE start");
      int16_t tone[FRAME_SAMPLES];
      float phase = 0;
      for (int f = 0; f < 66; f++) { // ~1 second
        for (int i = 0; i < FRAME_SAMPLES; i++) {
          phase += 2.0f * (float)M_PI * 440.0f / SAMPLE_RATE;
          tone[i] = (int16_t)(sinf(phase) * 12000);
        }
        sendFrame(tone, f == 0);
        delay(15); // pace like the mic would
      }
      Serial.println("D23_TESTTONE end");
      reportStatus();
    }
    if (c == 's') reportStatus();
  }
  delay(1);
}
