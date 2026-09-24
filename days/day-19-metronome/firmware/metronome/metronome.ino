// ESPtember Day 19 — Metronome
// Day 12 taught that clocks are arithmetic, not counters. Today that
// rule keeps time for music. Beats are scheduled on an absolute
// timeline — nextBeat += interval, never delay(interval) — so the
// tempo cannot drift no matter what rendering or buttons cost. Accented
// downbeat, haptic click you can feel with the sound off, tap-tempo on
// the second pusher, and a sweeping dot that bounces bar by bar.
#include <M5Unified.h>
#include <math.h>

// --- Transport ---------------------------------------------------------------
static bool running = false;
static uint16_t bpm = 120;
static uint8_t beatsPerBar = 4;
static uint8_t beatInBar = 0; // 0 = downbeat
static uint32_t nextBeatMs = 0;

static uint32_t beatInterval() { return 60000UL / bpm; }

// --- Tap tempo ---------------------------------------------------------------
// Four taps set the tempo from the median gap — the median shrugs off
// one sloppy tap the way an average can't. Taps more than two seconds
// apart start a fresh measurement.
#define TAPS 4
static uint32_t tapTimes[TAPS];
static int tapCount = 0;

static void tapTempo(uint32_t now) {
  if (tapCount && now - tapTimes[tapCount - 1] > 2000) tapCount = 0;
  if (tapCount < TAPS) tapTimes[tapCount++] = now;
  else {
    memmove(tapTimes, tapTimes + 1, sizeof(tapTimes) - sizeof(tapTimes[0]));
    tapTimes[TAPS - 1] = now;
  }
  if (tapCount < 2) return;
  uint32_t gaps[TAPS - 1];
  const int n = tapCount - 1;
  for (int i = 0; i < n; i++) gaps[i] = tapTimes[i + 1] - tapTimes[i];
  for (int i = 1; i < n; i++) // insertion sort: n is at most 3
    for (int j = i; j > 0 && gaps[j] < gaps[j - 1]; j--) {
      uint32_t t = gaps[j]; gaps[j] = gaps[j - 1]; gaps[j - 1] = t;
    }
  const uint32_t median = gaps[n / 2];
  int newBpm = (60000 + median / 2) / median;
  bpm = newBpm < 40 ? 40 : newBpm > 240 ? 240 : newBpm;
  if (running) nextBeatMs = now + beatInterval(); // re-anchor to the taps
}

// --- Click -------------------------------------------------------------------
static uint32_t vibeOffAt = 0;
static uint32_t lastBeatAt = 0;

static void click(bool downbeat) {
  M5.Speaker.tone(downbeat ? 1568 : 1046, 30, 0, true);
  M5.Power.setVibration(downbeat ? 220 : 120);
  vibeOffAt = millis() + (downbeat ? 50 : 30);
  lastBeatAt = millis();
}

static void updateVibe() {
  if (vibeOffAt && int32_t(millis() - vibeOffAt) >= 0) {
    M5.Power.setVibration(0);
    vibeOffAt = 0;
  }
}

// --- Display -------------------------------------------------------------------
static M5Canvas canvas(&M5.Display);

#define CX 233
#define CY 233

static void render() {
  const uint32_t now = millis();
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextDatum(middle_center);

  canvas.setTextColor(0xFD20, TFT_BLACK);
  canvas.setTextSize(2);
  char head[24];
  snprintf(head, sizeof(head), "%d/4", beatsPerBar);
  canvas.drawString(head, CX, 66);

  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setTextSize(7);
  char big[8];
  snprintf(big, sizeof(big), "%u", bpm);
  canvas.drawString(big, CX, 190);
  canvas.setTextSize(2);
  canvas.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  canvas.drawString("BPM", CX, 250);

  // The pendulum: a dot sweeping an arc, one pass per beat, direction
  // alternating like the arm of the mechanical original.
  const int arcR = 150;
  float phase = 0.5f;
  if (running) {
    const uint32_t interval = beatInterval();
    const uint32_t sinceBeat = now - (nextBeatMs - interval);
    phase = (float)(sinceBeat % interval) / interval;
    if (beatInBar % 2) phase = 1.0f - phase; // alternate sweep direction
  }
  const float ang = (float)M_PI * (0.25f + 0.5f * phase) + (float)M_PI / 2;
  canvas.drawArc(CX, CY + 40, arcR, arcR - 2, 135, 45, 0x39E7);
  const int dx = CX + (int)(cosf(ang) * arcR);
  const int dy = CY + 40 + (int)(sinf(ang) * arcR);
  const bool flash = now - lastBeatAt < 60;
  canvas.fillCircle(dx, dy, 14, flash ? 0x07E0 : 0xFD20);

  // Beat pips: filled = current beat in the bar.
  for (int i = 0; i < beatsPerBar; i++) {
    const int px = CX - (beatsPerBar - 1) * 18 + i * 36;
    if (running && i == beatInBar) canvas.fillCircle(px, 330, 10, 0x07E0);
    else canvas.drawCircle(px, 330, 10, TFT_DARKGRAY);
  }

  canvas.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  canvas.drawString(running ? "A: stop   B: tap" : "A: start  B: tap", CX, 388);
  canvas.drawString("A hold: beats/bar", CX, 414);
  canvas.pushSprite(0, 0);
}

static void reportStatus() {
  Serial.printf("D18_STATUS running=%d bpm=%u beats=%u beat=%u\n",
                (int)running, bpm, beatsPerBar, beatInBar);
}

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
  canvas.setColorDepth(8);
  canvas.createSprite(466, 466);
  Serial.println("D18_READY");
  reportStatus();
}

static uint32_t lastFrame = 0;

void loop() {
  M5.update();
  const uint32_t now = millis();

  if (M5.BtnA.wasClicked()) {
    running = !running;
    if (running) {
      beatInBar = 0;
      nextBeatMs = now; // first beat lands immediately
    } else {
      M5.Power.setVibration(0);
    }
    reportStatus();
  }
  if (M5.BtnA.wasHold()) {
    static const uint8_t bars[] = {2, 3, 4, 6};
    int i = 0;
    while (bars[i] != beatsPerBar) i++;
    beatsPerBar = bars[(i + 1) % 4];
    beatInBar = 0;
    reportStatus();
  }
  if (M5.BtnB.wasClicked()) {
    tapTempo(now);
    if (!running) M5.Speaker.tone(660, 20, 0, true); // count-in feedback
    reportStatus();
  }

  // The beat engine: absolute schedule, catch-up safe. If rendering
  // ever ran long, beats would fire late but the *timeline* would not
  // stretch — the next beat is computed from the last scheduled time,
  // never from "now".
  if (running && int32_t(now - nextBeatMs) >= 0) {
    click(beatInBar == 0);
    beatInBar = (beatInBar + 1) % beatsPerBar;
    nextBeatMs += beatInterval();
  }

  updateVibe();
  if (Serial.available() && Serial.read() == 's') reportStatus();
  if (now - lastFrame >= 33) {
    lastFrame = now;
    render();
  }
  delay(1);
}
