// ESPtember Day 20 — Tempo Checker
// The metronome's inverse: the mic listens, onsets are detected as
// jumps in short-frame energy, the gaps between onsets are folded into
// musical range, and the median inter-onset interval becomes a BPM
// with a confidence readout. Point it at day 19 and the pair validates
// itself — no reference instrument required.
#include <M5Unified.h>
#include <math.h>

// --- Capture ------------------------------------------------------------------
#define SAMPLE_RATE 16000
#define FRAME_N 256 // 16 ms per frame

static int16_t pcm[FRAME_N];

// --- Onset detection ------------------------------------------------------------
// A beat is a sudden rise in energy. Each 16 ms frame gets an RMS
// energy; an onset fires when energy jumps past 1.8x a slow running
// average AND at least 220 ms have passed since the last onset — the
// refractory window that keeps one drum hit from counting twice.
#define ONSETS 24
static uint32_t onsetAt[ONSETS];
static int onsetCount = 0;
static float slowAvg = 0;
static uint32_t lastOnset = 0;
static uint32_t lastOnsetFlash = 0;

static bool detectOnset(uint32_t now) {
  double sum = 0;
  float mean = 0;
  for (int i = 0; i < FRAME_N; i++) mean += pcm[i];
  mean /= FRAME_N;
  for (int i = 0; i < FRAME_N; i++) {
    const float v = pcm[i] - mean;
    sum += v * v;
  }
  const float energy = sqrtf(sum / FRAME_N);
  const bool fired = energy > slowAvg * 1.8f && energy > 120.0f &&
                     now - lastOnset > 220;
  // The average adapts slowly (2%) so the beat itself doesn't raise the
  // bar fast enough to hide its successors.
  slowAvg += 0.02f * (energy - slowAvg);
  if (!fired) return false;
  lastOnset = now;
  lastOnsetFlash = now;
  if (onsetCount < ONSETS) onsetAt[onsetCount++] = now;
  else {
    memmove(onsetAt, onsetAt + 1, sizeof(onsetAt) - sizeof(onsetAt[0]));
    onsetAt[ONSETS - 1] = now;
  }
  return true;
}

// --- BPM estimation ---------------------------------------------------------------
// Inter-onset gaps, each folded into the 40-240 BPM window (a missed
// beat reads as half tempo; folding doubles it back), then the median.
// Confidence = the fraction of folded gaps that agree with the median
// within 12%.
static uint16_t bpmEstimate = 0;
static uint8_t confidence = 0; // 0-100

static uint32_t foldGap(uint32_t ms) {
  while (ms > 1500) ms /= 2;  // < 40 BPM: assume missed beats
  while (ms < 250) ms *= 2;   // > 240 BPM: assume double-counted
  return ms;
}

static void estimate() {
  if (onsetCount < 5) { bpmEstimate = 0; confidence = 0; return; }
  uint32_t gaps[ONSETS - 1];
  int n = 0;
  for (int i = 1; i < onsetCount; i++) {
    const uint32_t g = onsetAt[i] - onsetAt[i - 1];
    if (g < 2500) gaps[n++] = foldGap(g);
  }
  if (n < 4) { bpmEstimate = 0; confidence = 0; return; }
  for (int i = 1; i < n; i++) // insertion sort
    for (int j = i; j > 0 && gaps[j] < gaps[j - 1]; j--) {
      uint32_t t = gaps[j]; gaps[j] = gaps[j - 1]; gaps[j - 1] = t;
    }
  const uint32_t median = gaps[n / 2];
  int agree = 0;
  for (int i = 0; i < n; i++) {
    const int32_t d = (int32_t)gaps[i] - (int32_t)median;
    if (labs(d) * 100 < (int32_t)median * 12) agree++;
  }
  bpmEstimate = (60000 + median / 2) / median;
  confidence = agree * 100 / n;
}

// --- Display --------------------------------------------------------------------
static M5Canvas canvas(&M5.Display);

#define CX 233
#define CY 233

static void render() {
  const uint32_t now = millis();
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextDatum(middle_center);

  canvas.setTextColor(0xFD20, TFT_BLACK);
  canvas.setTextSize(2);
  canvas.drawString("TEMPO", CX, 66);

  // Onset flash ring: every detected hit blinks the rim.
  if (now - lastOnsetFlash < 90) canvas.drawCircle(CX, CY, 226, 0x07E0);

  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setTextSize(7);
  char big[8];
  if (bpmEstimate) snprintf(big, sizeof(big), "%u", bpmEstimate);
  else snprintf(big, sizeof(big), "--");
  canvas.drawString(big, CX, 190);
  canvas.setTextSize(2);
  canvas.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  canvas.drawString("BPM", CX, 250);

  // The nodding dot: pulses on the estimated grid, projected forward
  // from the last onset — proof the estimate hears the same beat you do.
  if (bpmEstimate && confidence > 40) {
    const uint32_t interval = 60000UL / bpmEstimate;
    const uint32_t phase = (now - lastOnset) % interval;
    const float shrink = (float)phase / interval;
    canvas.fillCircle(CX, 320, 16 - (int)(10 * shrink), 0xFD20);
  }

  // Confidence bar.
  canvas.drawRect(CX - 100, 360, 200, 14, TFT_DARKGRAY);
  canvas.fillRect(CX - 100, 360, 2 * confidence, 14,
                  confidence > 60 ? 0x07E0 : 0xFD20);
  canvas.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  canvas.drawString("confidence", CX, 396);
  canvas.pushSprite(0, 0);
}

static void reportStatus() {
  Serial.printf("D19_STATUS bpm=%u conf=%u onsets=%d avg=%.0f\n", bpmEstimate,
                confidence, onsetCount, (double)slowAvg);
}

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = true;
  cfg.internal_mic = true;
  cfg.internal_spk = false; // mic day: the I2S engine listens only
  cfg.fallback_board = m5::board_t::board_M5StopWatch;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(255);
  canvas.setColorDepth(8);
  canvas.createSprite(466, 466);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Mic.begin();
  Serial.printf("D19_READY mic=%d\n", (int)M5.Mic.isEnabled());
}

static uint32_t lastFrame = 0, lastReport = 0;

void loop() {
  M5.update();
  const uint32_t now = millis();
  if (M5.Mic.record(pcm, FRAME_N, SAMPLE_RATE)) {
    if (detectOnset(now)) estimate();
  }
  // B click clears the measurement for a fresh song.
  if (M5.BtnB.wasClicked()) {
    onsetCount = 0;
    bpmEstimate = 0;
    confidence = 0;
  }
  if (now - lastReport >= 2000) { lastReport = now; reportStatus(); }
  if (Serial.available() && Serial.read() == 's') reportStatus();
  if (now - lastFrame >= 40) { lastFrame = now; render(); }
}
