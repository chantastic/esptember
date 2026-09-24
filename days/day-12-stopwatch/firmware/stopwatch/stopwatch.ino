// ESPtember Day 12 — Basic-Ass Stopwatch
// The kit is shaped like a stopwatch. Today it behaves like one, with
// the pusher ergonomics every mechanical stopwatch and Casio digital
// agrees on: the crown starts and stops — and never resets. The second
// pusher laps while running and resets while stopped, and reset
// requires a hold, because an accidental reset is the one unforgivable
// stopwatch bug. Builds on day 11's gestures and feedback.
#include <M5Unified.h>

// --- Timekeeping --------------------------------------------------------
// One rule: the display never owns the time. Elapsed time is computed
// from millis() deltas against an accumulated base, so rendering rate,
// button handling, and serial chatter can never skew the clock.

static bool running = false;
static uint32_t startedAt = 0;   // millis() at last start
static uint32_t accumulated = 0; // ms banked across stops

static uint32_t elapsedMs() {
  return running ? accumulated + (millis() - startedAt) : accumulated;
}

// --- Laps ---------------------------------------------------------------
// Lap freezes a split on screen; the underlying timer never pauses.
#define MAX_LAPS 99
static uint32_t laps[MAX_LAPS];
static int lapCount = 0;

static void takeLap() {
  if (lapCount < MAX_LAPS) laps[lapCount++] = elapsedMs();
}

// --- Feedback (day 11's vocabulary) --------------------------------------
static uint32_t vibeOffAt = 0;

static void cue(uint16_t hz, uint16_t toneMs, uint8_t vibe, uint16_t vibeMs) {
  M5.Speaker.tone(hz, toneMs, 0, true);
  M5.Power.setVibration(vibe);
  vibeOffAt = millis() + vibeMs;
}

static void updateVibe() {
  if (vibeOffAt && int32_t(millis() - vibeOffAt) >= 0) {
    M5.Power.setVibration(0);
    vibeOffAt = 0;
  }
}

// --- Display --------------------------------------------------------------
static void formatTime(char *out, size_t n, uint32_t ms) {
  snprintf(out, n, "%02u:%02u.%02u", (unsigned)(ms / 60000),
           (unsigned)(ms / 1000 % 60), (unsigned)(ms / 10 % 100));
}

static void render() {
  char buf[16];
  M5.Display.startWrite();
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);

  M5.Display.setTextColor(running ? 0x07E0 : 0xFD20, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.drawString(running ? "RUNNING" : "STOPPED", 233, 84);

  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(5);
  formatTime(buf, sizeof(buf), elapsedMs());
  M5.Display.drawString(buf, 233, 170);

  // Last three laps, newest first.
  M5.Display.setTextSize(2);
  for (int i = 0; i < 3 && i < lapCount; i++) {
    const int index = lapCount - 1 - i;
    char line[24];
    formatTime(buf, sizeof(buf), laps[index]);
    snprintf(line, sizeof(line), "LAP %d  %s", index + 1, buf);
    M5.Display.setTextColor(i == 0 ? TFT_WHITE : TFT_DARKGRAY, TFT_BLACK);
    M5.Display.drawString(line, 233, 250 + i * 36);
  }

  M5.Display.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.drawString(running ? "B: lap" : "B hold: reset", 233, 386);
  M5.Display.endWrite();
}

// --- Serial status (development aid) ---------------------------------------
static void reportStatus() {
  Serial.printf("D11_STATUS running=%d elapsed=%u laps=%d\n", (int)running,
                (unsigned)elapsedMs(), lapCount);
}

static uint32_t lastDraw = 0;

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = true;
  cfg.internal_mic = false;
  cfg.internal_spk = true;
  cfg.fallback_board = m5::board_t::board_M5StopWatch;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(255);
  M5.Power.setVibration(0);
  render();
  Serial.println("D11_READY");
  reportStatus();
}

void loop() {
  M5.update();
  bool dirty = false;

  // Crown (A): start/stop. Never resets — a running timer is sacred.
  if (M5.BtnA.wasClicked()) {
    if (running) {
      accumulated += millis() - startedAt;
      running = false;
      cue(660, 120, 160, 120); // stop: lower
    } else {
      startedAt = millis();
      running = true;
      cue(880, 80, 120, 60); // start: short, high
    }
    dirty = true;
    reportStatus();
  }

  // Second pusher (B): lap while running, hold-to-reset while stopped.
  if (running && M5.BtnB.wasClicked()) {
    takeLap();
    cue(1046, 60, 120, 50); // lap: quick chirp
    dirty = true;
    reportStatus();
  }
  if (!running && M5.BtnB.wasHold()) {
    accumulated = 0;
    lapCount = 0;
    cue(330, 300, 255, 300); // reset: long and low — the deliberate one
    dirty = true;
    reportStatus();
  }

  updateVibe();
  if (Serial.available() && Serial.read() == 's') reportStatus();

  // 10 Hz refresh while running; event-driven otherwise.
  if (dirty || (running && uint32_t(millis() - lastDraw) >= 100)) {
    render();
    lastDraw = millis();
  }
  delay(5);
}
