// ESPtember Day 15 — Music Visualizer
// The mic listens, a 256-point FFT splits the room into frequencies,
// and 24 bars ring the round face like a radial equalizer — bass at
// twelve o'clock, treble wrapping around. Bars rise instantly and fall
// slowly, the decay every hardware visualizer has used since the
// graphic-EQ era. M5Unified's speaker and mic share the I2S engine, so
// today the board only listens.
#include <M5Unified.h>
#include <math.h>

// --- Capture -----------------------------------------------------------------
#define SAMPLE_RATE 16000
#define FFT_N 256 // 62.5 Hz per bin at 16 kHz

static int16_t pcm[FFT_N];

// --- FFT ------------------------------------------------------------------
// A plain radix-2 Cooley-Tukey, written out instead of imported: it's
// thirty lines, and seeing it is the lesson. Hann window first — a
// rectangular window smears every note across the spectrum.
static float re[FFT_N], im[FFT_N], hann[FFT_N];

static void fftInit() {
  for (int i = 0; i < FFT_N; i++)
    hann[i] = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * i / (FFT_N - 1)));
}

static void fft() {
  // bit-reversal permutation
  for (int i = 1, j = 0; i < FFT_N; i++) {
    int bit = FFT_N >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) {
      float t = re[i]; re[i] = re[j]; re[j] = t;
      t = im[i]; im[i] = im[j]; im[j] = t;
    }
  }
  // butterflies
  for (int len = 2; len <= FFT_N; len <<= 1) {
    const float ang = -2.0f * (float)M_PI / len;
    const float wr = cosf(ang), wi = sinf(ang);
    for (int i = 0; i < FFT_N; i += len) {
      float cr = 1, ci = 0;
      for (int k = 0; k < len / 2; k++) {
        const int a = i + k, b = i + k + len / 2;
        const float tr = re[b] * cr - im[b] * ci;
        const float ti = re[b] * ci + im[b] * cr;
        re[b] = re[a] - tr; im[b] = im[a] - ti;
        re[a] += tr; im[a] += ti;
        const float ncr = cr * wr - ci * wi;
        ci = cr * wi + ci * wr; cr = ncr;
      }
    }
  }
}

// --- Bands -------------------------------------------------------------------
// 24 bars from ~125 Hz to 8 kHz on a log scale — equal notes per bar,
// the spacing ears actually hear. Each bar: peak magnitude of its bin
// range, converted to dB, mapped to length.
#define BARS 24
static uint8_t barLevel[BARS]; // 0..100, after rise/decay dynamics
static int barLo[BARS], barHi[BARS];

static void bandsInit() {
  const float fLo = 125.0f, fHi = 8000.0f, binHz = (float)SAMPLE_RATE / FFT_N;
  for (int b = 0; b < BARS; b++) {
    const float lo = fLo * powf(fHi / fLo, (float)b / BARS);
    const float hi = fLo * powf(fHi / fLo, (float)(b + 1) / BARS);
    barLo[b] = (int)fmaxf(1, lo / binHz);
    barHi[b] = (int)fminf(FFT_N / 2 - 1, hi / binHz);
    if (barHi[b] < barLo[b]) barHi[b] = barLo[b];
  }
}

static void analyze() {
  // Remove the DC offset first — a MEMS mic's bias otherwise leaks
  // through the window into the low bins and pins the bass bars.
  float mean = 0;
  for (int i = 0; i < FFT_N; i++) mean += pcm[i];
  mean /= FFT_N;
  for (int i = 0; i < FFT_N; i++) {
    re[i] = (pcm[i] - mean) * hann[i];
    im[i] = 0;
  }
  fft();
  for (int b = 0; b < BARS; b++) {
    float peak = 0;
    for (int k = barLo[b]; k <= barHi[b]; k++) {
      const float mag = sqrtf(re[k] * re[k] + im[k] * im[k]);
      if (mag > peak) peak = mag;
    }
    // ~40 dB of display range above the noise floor.
    const float db = 20.0f * log10f(peak + 1.0f);
    int level = (int)((db - 62.0f) * 100.0f / 36.0f);
    level = level < 0 ? 0 : level > 100 ? 100 : level;
    // Instant rise, slow fall.
    if (level > barLevel[b]) barLevel[b] = level;
    else barLevel[b] = barLevel[b] > 7 ? barLevel[b] - 7 : 0;
  }
}

// --- Rendering ------------------------------------------------------------------
static M5Canvas canvas(&M5.Display);
static uint32_t tPush0;

// The sprite covers only the ring's bounding box (420x420) at 8-bit
// color: 176 KB per push instead of 434 KB — the difference between a
// visible wipe and a live meter.
#define SPRITE_SIZE 420
#define SPRITE_OFF ((466 - SPRITE_SIZE) / 2)
#define CX (SPRITE_SIZE / 2)
#define CY (SPRITE_SIZE / 2)
#define R_IN 90
#define R_MAX 208

static void render() {
  canvas.fillSprite(TFT_BLACK);
  canvas.fillCircle(CX, CY, R_IN - 14, 0x1082);
  for (int b = 0; b < BARS; b++) {
    // Bass at twelve o'clock, wrapping clockwise.
    const float ang = -(float)M_PI / 2 + 2.0f * (float)M_PI * b / BARS;
    const float len = R_IN + (R_MAX - R_IN) * barLevel[b] / 100.0f;
    // Color rides the level: dim orange floor to white-hot peaks.
    const uint8_t glow = 120 + barLevel[b];
    const uint16_t color = canvas.color565(glow > 255 ? 255 : glow,
                                           40 + barLevel[b], barLevel[b] / 2);
    const float c = cosf(ang), s = sinf(ang);
    // A bar is a thick radial line: three parallel strokes.
    for (int off = -2; off <= 2; off++) {
      const float ox = -s * off, oy = c * off;
      canvas.drawLine(CX + (int)(c * R_IN + ox), CY + (int)(s * R_IN + oy),
                      CX + (int)(c * len + ox), CY + (int)(s * len + oy),
                      color);
    }
  }
  tPush0 = millis();
  canvas.pushSprite(SPRITE_OFF, SPRITE_OFF);
}

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = true;
  cfg.internal_mic = true;
  cfg.internal_spk = false; // mic and speaker share the I2S engine
  cfg.fallback_board = m5::board_t::board_M5StopWatch;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(255);
  canvas.setColorDepth(8);
  canvas.createSprite(SPRITE_SIZE, SPRITE_SIZE);
  M5.Display.fillScreen(TFT_BLACK);
  fftInit();
  bandsInit();
  M5.Mic.begin();
  Serial.printf("D15_READY mic=%d\n", (int)M5.Mic.isEnabled());
}

static uint32_t lastReport = 0;

void loop() {
  M5.update();
  static uint32_t tRec = 0, tFft = 0, tDraw = 0, tPush = 0, frames = 0;
  uint32_t t0 = millis();
  if (M5.Mic.record(pcm, FFT_N, SAMPLE_RATE)) {
    uint32_t t1 = millis();
    analyze();
    uint32_t t2 = millis();
    render();
    uint32_t t3 = millis();
    tRec += t1 - t0; tFft += t2 - t1; tDraw += tPush0 - t2;
    tPush += t3 - tPush0; frames++;
  }
  if (millis() - lastReport >= 2000) {
    if (frames) Serial.printf("D15_TIMING fps=%u rec=%u fft=%u draw=%u push=%u (ms avg)\n",
                              frames / 2, tRec / frames, tFft / frames,
                              tDraw / frames, tPush / frames);
    tRec = tFft = tDraw = tPush = frames = 0;
    lastReport = millis();
    int sum = 0, top = 0, topBar = 0;
    for (int b = 0; b < BARS; b++) {
      sum += barLevel[b];
      if (barLevel[b] > top) { top = barLevel[b]; topBar = b; }
    }
    Serial.printf("D15_LEVELS avg=%d top=%d topbar=%d\n", sum / BARS, top,
                  topBar);
  }
}
