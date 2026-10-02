// Pi Pet for the M5Stack Stopwatch (disposable candidate, v3).
// A port of the Grok Bot avatar motion system (x.ai "Designing Grok Bot"):
// eight roster heads, 25 morphing eye expressions, per-state spring motion,
// blinks and winks, bounce chains, 3D eye spins, and glyph morphs (thinking
// dots, blocked "!"). Each running pi session gets one pet.
//
// Controls (lanyard at the bottom): left (BtnA) / right (BtnB) page sessions
// (demo: bots); both briefly = acknowledge (demo: next state); both held
// 600 ms = sleep. Touch: tap the pet to bounce, drag it, tap elsewhere to look.
#pragma GCC optimize("O2")
#include <M5Unified.h>
#include <Preferences.h>
#include <math.h>
#include "buttons.h"
#include "types.h"
#include "touch_calibration_core.h"
#include "grok_assets.h"
#include "net_link.h"

static constexpr int W = 468, H = 466;
static M5Canvas canvas(&M5.Display);
static uint16_t *fb = nullptr;

// ---------- touch map (shared espt-touch/record) ----------
static touchcal::WarpMap touchMap{};
static uint16_t touchMapVersion = 0;
static uint32_t touchMapGeneration = 0;

static bool loadTouchMap() {
  Preferences prefs;
  if (!prefs.begin(touchcal::kNvsNamespace, true)) return false;
  const size_t length = prefs.getBytesLength("record");
  touchcal::RecordV2 v2{};
  touchcal::Record v1{};
  size_t read = 0;
  if (length == sizeof(v2)) read = prefs.getBytes("record", &v2, sizeof(v2));
  else if (length == sizeof(v1)) read = prefs.getBytes("record", &v1, sizeof(v1));
  prefs.end();
  if (read == sizeof(v2) && touchcal::validateRecordV2(v2)) {
    touchMap.affine = {v2.affine[0], v2.affine[1], v2.affine[2], v2.affine[3], v2.affine[4], v2.affine[5]};
    touchMap.center = {v2.centerResidual[0], v2.centerResidual[1]};
    for (size_t r = 0; r < touchcal::kRadialRingCount; ++r)
      for (size_t s = 0; s < touchcal::kPerimeterSectorCount; ++s)
        touchMap.rings[r][s] = {v2.residuals[r][s][0], v2.residuals[r][s][1]};
    touchMapVersion = 2;
    touchMapGeneration = v2.generation;
    return true;
  }
  if (read == sizeof(v1) && touchcal::validateRecord(v1)) {
    touchMap = touchcal::warpFromV1(v1);
    touchMapVersion = 1;
    return true;
  }
  return false;
}

// ---------- math ----------
static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline float mixf(float a, float b, float t) { return a + (b - a) * t; }
static inline float frand(float a, float b) { return a + (b - a) * (esp_random() / 4294967295.0f); }
static inline uint32_t urand(uint32_t a, uint32_t b) { return a + uint32_t(frand(0, 1) * (b - a)); }
static inline float smooth(float e) { return e * e * (3 - 2 * e); }
// sqrt via one-Newton-step inverse sqrt: ~0.2% error, plenty for antialiased coverage.
static inline float fsqrt(float x) {
  if (x <= 0) return 0;
  union { float f; uint32_t i; } u = {x};
  u.i = 0x5f3759df - (u.i >> 1);
  const float y = u.f * (1.5f - 0.5f * x * u.f * u.f);
  return x * y;
}
static RGB rgb(uint32_t hex) { return {float((hex >> 16) & 255), float((hex >> 8) & 255), float(hex & 255)}; }

// Damped spring in the web component's form: omega (rad/s) and damping ratio.
struct Spring {
  float x = 0, v = 0, t = 0;
  void init(float value) { x = t = value; v = 0; }
  void step(float w, float z, float h) {
    v += (-2 * z * w * v - w * w * (x - t)) * h;
    x += v * h;
    if (!isfinite(x) || !isfinite(v)) { x = t; v = 0; }
  }
};

// ---------- roster ----------
static constexpr float C = 114.2705f;           // head units: 228.54 box centred here
static constexpr float kScale = 0.92f;          // screen px per head unit
static constexpr float kHomeX = 234;
// Vertical home: above centre when the name/activity band shows (pets), centred without it (demo).
static constexpr float kHomeYBand = 204, kHomeYCentred = 233;
static float kHomeY = kHomeYCentred;  // eased each tick toward the target
static constexpr uint32_t kBg = 0x000000, kLabel = 0xB9B4A8;  // pure black: AMOLED pixels off
struct Style { const char *name; uint8_t head; uint32_t ink; };
static const Style kStyles[] = {
  {"Blob", 0, 0x1084FE}, {"Pebble", 1, 0xFF6700}, {"Squircle", 2, 0x00BCA6}, {"Tablet", 3, 0xFF263C},
  {"Wedge", 4, 0xFF309B}, {"Hex", 5, 0x9159FE}, {"Cloud", 6, 0xFF9800}, {"Teardrop", 7, 0x97683D},
};
static constexpr int kStyleCount = sizeof(kStyles) / sizeof(kStyles[0]);
static const uint32_t kConfetti[] = {0xF9705C, 0x5B95F0, 0x3FBE86, 0xF5B13F, 0x9A72EE, 0x35C3BD};

// ---------- avatar states (web engine names in comments) ----------
enum EState { SIdle, SThinking, SWorking, SWaiting, SBlocked, SDone, SSurprised, SSleeping,
              SHappy, SCurious, SExcited, SSad, kStateCount };
static const char *const kStateNames[kStateCount] = {
  "idle", "thinking", "working", "waiting", "blocked", "done", "surprised", "sleeping",
  "happy", "curious", "excited", "sad"};
enum Glyph : uint8_t { GNone, GDots, GBang };
struct StateDef {
  uint8_t expr[5], nExpr;
  uint16_t exprMin, exprMax, blinkMin, blinkMax;  // blinkMax 0 = no blinks
  float gx0, gx1, gy0, gy1; bool gxSigned;         // gaze: x = 15*[gx0,gx1] (random sign), y = 9*[gy0,gy1]
  uint16_t gazeMin, gazeMax;
  uint8_t exprSpeed; Glyph glyph; bool winks;
};
static const StateDef kStates[kStateCount] = {
  /* idle      */ {{0, 8}, 2, 9000, 16000, 6000, 14000, 0, 0, 0, 0, false, 2500, 5500, 6, GNone, true},
  /* thinking  */ {{8, 16, 14, 17, 5}, 5, 2000, 3600, 3500, 7000, .5f, 1, -1, -.4f, true, 1500, 2800, 6, GDots, false},
  /* working   */ {{7, 16, 11, 10}, 4, 1800, 3200, 2800, 5500, -.4f, .4f, .4f, 1, false, 1200, 2400, 6, GNone, false},
  /* waiting   */ {{4, 22, 0}, 3, 3500, 6000, 4000, 8000, .7f, 1, .4f, .9f, true, 3000, 6000, 6, GNone, false},  // bored
  /* blocked   */ {{3, 21}, 2, 2000, 3600, 0, 0, -.4f, .4f, -.3f, .3f, false, 2500, 5000, 6, GBang, false},     // alerting
  /* done      */ {{2, 8, 17}, 3, 1400, 2600, 2200, 4500, -.4f, .4f, -.3f, .3f, false, 2500, 5000, 7, GNone, false},  // celebrate
  /* surprised */ {{3, 21}, 2, 2500, 4000, 1800, 3500, 0, 0, 0, 0, false, 1600, 2600, 7, GNone, false},
  /* sleeping  */ {{13, 22, 4}, 3, 6000, 10000, 0, 0, 0, 0, 0, 0, false, 2500, 5000, 6, GNone, false},
  /* happy     */ {{2, 11, 17, 19}, 4, 2500, 4500, 2500, 5000, -.7f, .7f, -.6f, 0, false, 1800, 3400, 7, GNone, true},
  /* curious   */ {{3, 21, 0, 15}, 4, 1800, 3200, 2500, 5500, .6f, 1, -1, 1, true, 950, 1900, 7, GNone, true},
  /* excited   */ {{2, 17, 21, 3, 11}, 5, 1100, 2000, 2000, 4000, -1, 1, -1, .3f, false, 700, 1400, 10, GNone, true},
  /* sad       */ {{4, 13, 22}, 3, 4000, 7000, 4000, 8000, -.3f, .3f, .6f, 1, false, 2800, 5000, 6, GNone, false},
};
static int state = SIdle;
static int demoState = SIdle;  // demo look, restored when the last pet leaves
static int demoStyle = 0;
static int styleIndex = 0;

// ---------- animation state ----------
static Spring d, o, A, h;           // rotation (deg), x offset, y offset (units), vertical scale
static Spring lid, eyeScale;        // eyelid openness, eye size
static Spring gazeX, gazeY;         // eye offset (units, +-15 / +-9)
static Spring blend;                // expression morph 0 -> 1
static Spring glyphMix;             // glyph presence: the head contracts into the glyph (w 14)
static Spring styleMix;             // 0 = thinking dots, 1 = blocked "!" (w 11, direct glyph changes)
static Spring eyeTurn;              // cylindrical eye half-turns on glyph entry/exit (w 14)
static float eyeTurnDir = 1;
static bool glyphPresent = false;
static uint32_t bangT0 = 0;         // separate pulse clocks per glyph (study adaptation)
static Spring headMix;              // previous head -> current head SDF morph
static Spring spin;                 // eyes travelling around the head (radians)
static Spring inkR, inkG, inkB;
static Spring sqx, sqy, dragX, dragY;  // touch squash and drag offset (screen px)
static bool spinning = false;
static float spinAxis = 0;          // spin axis angle from vertical (radians, screen plane)
static int spinQueue = 0;           // extra spins still to run (done/celebrate)
struct Ribbon { float tilt, roll, phase, speed, len, radius, born, life; uint32_t color; };
static Ribbon ribbons[9]; static int ribbonCount = 0;
static int pendingExpr = -1; static uint32_t pendingAt = 0; static uint8_t pendingSpeed = 6;
static int headPrev = 0, headCur = 0;
static int8_t sdfCur[kSdfN * kSdfN], sdfPrev[kSdfN * kSdfN];  // internal RAM copies (flash reads are slow)
// Active expression's contour SDF textures (exact distance to the 48-point contours), in RAM.
static int8_t eyeTex[2][7680];
static int eyeTexW[2], eyeTexH[2], eyeTexExpr = -1;
static float eyeTexX0[2], eyeTexY0[2];
static void loadEyeTex(int e) {
  if (e == eyeTexExpr) return;
  for (int i = 0; i < 2; ++i) {
    const int32_t *inf = kEyeTexInfo[e][i];
    eyeTexW[i] = inf[1]; eyeTexH[i] = inf[2]; eyeTexX0[i] = inf[3]; eyeTexY0[i] = inf[4];
    memcpy(eyeTex[i], kEyeTex + inf[0], size_t(inf[1]) * inf[2]);
  }
  eyeTexExpr = e;
}
static inline float sampleEyeTex(int i, float x, float y) {
  const float fx = x - eyeTexX0[i], fy = y - eyeTexY0[i];
  const int w = eyeTexW[i], h = eyeTexH[i];
  if (fx < 0 || fy < 0 || fx >= w - 1 || fy >= h - 1) return 31.75f;
  const int ix = int(fx), iy = int(fy);
  const float tx = fx - ix, ty = fy - iy;
  const int8_t *p = eyeTex[i] + iy * w + ix;
  const float a = p[0] + (p[1] - p[0]) * tx, b = p[w] + (p[w + 1] - p[w]) * tx;
  return (a + (b - a) * ty) * (1 / kSdfQuant);
}
static Cap exprFrom[2], exprTo[2];
static int exprIndex = 0;
static uint8_t exprSpeed = 6;
static uint32_t stateSince = 0, nextExpr = 0, nextBlink = 0, nextGaze = 0, nextSpecial = 0, nextWink = 0;
static uint32_t nextSigh = 0, sighAt = 0, nextTilt = 0, tiltPulseAt = 0, gazeHoldUntil = 0;
static uint32_t lidKeyAt[6]; static float lidKeyV[6]; static int lidKeys = 0;
static uint32_t winkAt = 0; static int winkEye = 0;
static int32_t bounceStart = -1;
static uint32_t dotsPhase0 = 0;
static bool badge = false;
static Particle parts[28]; static int partCount = 0;
static uint32_t labelUntil = 0; static char labelText[32] = "";

// Touch
static bool touchDown = false, dragging = false;
static float touchX = 0, touchY = 0, touchStartX = 0, touchStartY = 0;
static uint32_t touchStartMs = 0, lastTouchMs = 0;
static float grabDX = 0, grabDY = 0;

// IMU
static float tiltX = 0, tiltY = 0;
static bool imuOk = false;
static uint32_t lastImuLog = 0;

static c25k::ButtonGrammar buttons;

static void showLabel(const char *text) {
  strncpy(labelText, text, sizeof(labelText) - 1);
  labelUntil = millis() + 1600;
}

static Cap exprCap(int e, int eye) {
  const float *v = kExpr[e][eye];
  return {v[0], v[1], v[2], v[3], v[4]};
}

// Current morphed expression (angles take the nearest equivalent: capsules are symmetric).
static Cap blendedCap(int eye) {
  const float t = clampf(blend.x, 0, 1);
  const Cap &a = exprFrom[eye], &b = exprTo[eye];
  float db = b.ang - a.ang;
  while (db > PI / 2) db -= PI;
  while (db < -PI / 2) db += PI;
  return {mixf(a.cx, b.cx, t), mixf(a.cy, b.cy, t), a.ang + db * t, mixf(a.len, b.len, t), mixf(a.wid, b.wid, t)};
}

// Web look: an expression change closes the eyes to dashes, swaps while shut, then pops open.
static void blinkNow(uint32_t now);
static void changeExpr(int e, uint8_t speed, uint32_t now) {
  if (e == exprIndex) return;
  blinkNow(now);
  pendingExpr = e; pendingSpeed = speed; pendingAt = now + 80;
}
static void setExpr(int e, uint8_t speed) {
  if (e == exprIndex && blend.x > 0.99f) return;
  exprFrom[0] = blendedCap(0); exprFrom[1] = blendedCap(1);
  exprTo[0] = exprCap(e, 0); exprTo[1] = exprCap(e, 1);
  exprIndex = e; exprSpeed = speed;
  blend.x = 0; blend.v = 0; blend.t = 1;
}

static void blinkNow(uint32_t now) {  // web keyframes: shut, hold, overshoot open, settle
  lidKeys = 0;
  auto key = [&](uint32_t at, float v) { if (lidKeys < 6) { lidKeyAt[lidKeys] = at; lidKeyV[lidKeys++] = v; } };
  key(now, .05f); key(now + 70, .05f); key(now + 150, 1.08f); key(now + 300, 1);
  if (frand(0, 1) < .14f) { key(now + 370, .05f); key(now + 480, 1); }
}

static void startBounce() { if (bounceStart < 0) bounceStart = int32_t(millis()); }
// Ribbons travel in lanes: a few parallel bands share one tilted orbit, each its own colour.
static void spawnRibbons(int n) {
  const float now = millis() / 1000.0f;
  const float tilt = frand(.9f, 1.45f), roll = frand(-PI, PI), phase = frand(0, 2 * PI);
  const float speed = frand(4.5f, 6.5f) * (frand(0, 1) < .5f ? 1 : -1), len = frand(1.6f, 2.4f);
  const int c0 = esp_random() % 6;
  for (int i = 0; i < n; ++i) {
    const bool newPlane = i > 0 && i % 4 == 0;  // more than four lanes: start another orbit plane
    Ribbon r{newPlane ? frand(.9f, 1.45f) : tilt, newPlane ? frand(-PI, PI) : roll, phase - i * .12f, speed,
             len - i * .1f, 116 + (i % 4) * 7.5f, now + i * .04f, frand(2.0f, 2.6f), kConfetti[(c0 + i) % 6]};
    if (ribbonCount < 9) ribbons[ribbonCount++] = r;
    else ribbons[esp_random() % 9] = r;
  }
}
// Eyes travel around the head about a (possibly tilted) axis; orbit ribbons trail the spin.
static void startSpin(float turns = 1, float axis = 0, int ribbonsToAdd = 5) {
  if (spinning) return;
  spinning = true;
  spinAxis = axis;
  spin.x = 0; spin.v = 0; spin.t = turns * 2 * PI * (frand(0, 1) < .5f ? 1 : -1);
  if (ribbonsToAdd) spawnRibbons(ribbonsToAdd);
}
static void burst(int n) {
  for (int i = 0; i < n && partCount < 28; ++i) {
    const float a = frand(-PI, 0), sp = frand(220, 520);
    parts[partCount++] = {kHomeX + dragX.x, kHomeY + dragY.x - 60, cosf(a) * sp, sinf(a) * sp - 120,
                          frand(0, 6.3f), frand(-9, 9), 0, frand(9, 15), kConfetti[esp_random() % 6]};
  }
}

static void applyStyle(int index, bool announce) {
  styleIndex = (index % kStyleCount + kStyleCount) % kStyleCount;
  const Style &s = kStyles[styleIndex];
  if (s.head != headCur) {
    headPrev = headCur; headCur = s.head; headMix.x = 0; headMix.v = 0; headMix.t = 1;
    memcpy(sdfPrev, sdfCur, sizeof(sdfCur));
    memcpy(sdfCur, kHeadSdf[headCur], sizeof(sdfCur));
  }
  const RGB c = rgb(s.ink);
  inkR.t = c.r; inkG.t = c.g; inkB.t = c.b;
  sqy.v -= 3.5f; sqx.v += 3.0f;
  if (announce) showLabel(s.name);
  netOut.printf("GB_EVENT style=%s\n", s.name);
}

static void enterState(int s, bool announce) {
  if (s == state) return;
  state = s;
  const StateDef &def = kStates[s];
  const uint32_t now = millis();
  stateSince = now;
  changeExpr(def.expr[esp_random() % def.nExpr], def.exprSpeed, now);
  nextExpr = now + urand(def.exprMin, def.exprMax);
  nextBlink = now + urand(1500, 7000);
  nextGaze = now + urand(500, 1400);
  nextSpecial = now + (s == SExcited ? urand(400, 1100) : s == SWorking ? urand(1200, 2400) : urand(9000, 18000));
  nextSigh = now + urand(1500, 3500);
  nextTilt = now + urand(1200, 2200);
  nextWink = now + urand(3000, 8000);
  // Brightness is managed centrally in updateBrightness().
  if (s == SDone) { spinning = false; startSpin(2, frand(-.9f, .9f), 9); spinQueue = 2; startBounce(); }
  if (s == SThinking) dotsPhase0 = now;
  if (s == SBlocked) bangT0 = now;
  if (announce) showLabel(kStateNames[s]);
  netOut.printf("GB_EVENT mood=%s\n", kStateNames[s]);
}

// ---------- transition sounds (synthesized at boot, 22.05 kHz mono) ----------
enum SoundId { SndPop, SndStart, SndChime, SndWince, SndUhoh, kSoundCount };
static const char *const kSoundNames[kSoundCount] = {"pop", "start", "chime", "wince", "uhoh"};
static int16_t *soundPcm[kSoundCount];
static size_t soundLen[kSoundCount];
static constexpr uint32_t kSoundRate = 22050;
static bool soundMuted();  // manual sleep mutes

// A tone with a short attack and exponential decay; partials add bell colour.
static void addTone(float *buf, size_t n, float t0, float dur, float f0, float f1, float amp, float tau,
                    float partial = 0, float partialAmp = 0) {
  const size_t a = size_t(t0 * kSoundRate), len = size_t(dur * kSoundRate);
  float ph = 0, ph2 = 0;
  for (size_t i = 0; i < len && a + i < n; ++i) {
    const float t = float(i) / kSoundRate, k = float(i) / len;
    const float f = f0 + (f1 - f0) * k;
    ph += 2 * PI * f / kSoundRate; ph2 += 2 * PI * f * partial / kSoundRate;
    const float env = fminf(t / .004f, 1) * expf(-t / tau) * fminf((len - i) / (.006f * kSoundRate), 1);
    buf[a + i] += amp * env * (sinf(ph) + partialAmp * sinf(ph2));
  }
}
static void makeSound(int id, float seconds, void (*fill)(float *, size_t)) {
  const size_t n = size_t(seconds * kSoundRate);
  float *tmp = static_cast<float *>(ps_malloc(n * sizeof(float)));
  soundPcm[id] = static_cast<int16_t *>(ps_malloc(n * sizeof(int16_t)));
  if (!tmp || !soundPcm[id]) { soundLen[id] = 0; free(tmp); return; }
  memset(tmp, 0, n * sizeof(float));
  fill(tmp, n);
  float peak = 1e-6f;
  for (size_t i = 0; i < n; ++i) peak = fmaxf(peak, fabsf(tmp[i]));
  for (size_t i = 0; i < n; ++i) soundPcm[id][i] = int16_t(tmp[i] / peak * 15000);  // ~-6.8 dBFS
  soundLen[id] = n;
  free(tmp);
}
static void initSounds() {
  makeSound(SndPop, .09f, [](float *b, size_t n) { addTone(b, n, 0, .09f, 520, 1100, 1, .035f); });
  makeSound(SndStart, .05f, [](float *b, size_t n) { addTone(b, n, 0, .05f, 1760, 1900, 1, .012f); });
  makeSound(SndChime, .75f, [](float *b, size_t n) {  // C6 E6 G6 bell arpeggio
    addTone(b, n, 0.00f, .60f, 1046.5f, 1046.5f, 1, .20f, 2.76f, .25f);
    addTone(b, n, 0.09f, .60f, 1318.5f, 1318.5f, .9f, .20f, 2.76f, .25f);
    addTone(b, n, 0.18f, .57f, 1568.0f, 1568.0f, .85f, .24f, 2.76f, .25f);
  });
  makeSound(SndWince, .13f, [](float *b, size_t n) { addTone(b, n, 0, .13f, 470, 330, 1, .06f, 2, .3f); });
  makeSound(SndUhoh, .42f, [](float *b, size_t n) {  // "uh-oh": G4 then D4
    addTone(b, n, 0.00f, .15f, 392, 392, 1, .10f, 3, .25f);
    addTone(b, n, 0.18f, .24f, 294, 290, 1, .14f, 3, .25f);
  });
}
static void playSound(int id) {
  if (soundMuted() || id < 0 || id >= kSoundCount || !soundLen[id]) return;
  M5.Speaker.playRaw(soundPcm[id], soundLen[id], kSoundRate, false, 1, 0, true);
  netOut.printf("GB_SOUND %s\n", kSoundNames[id]);
}

// ---------- pets: one per pi session, fed by the host hub ----------
static const char *const kPetStateNames[] = {"idle", "thinking", "tool", "error", "done", "waiting", "sleeping"};
static constexpr int kMaxPets = 8;
static Pet pets[kMaxPets];
static int focus = -1;  // -1 = demo mode (no hub data)
static bool manualSleep = false;
// Silent in manual sleep and whenever there are no sessions (the demo is the screensaver).
static bool soundMuted() { return manualSleep || focus < 0; }
static uint32_t lastHubMs = 0;        // last line received from the hub (USB or Wi-Fi)
static uint32_t lastInteractionMs = 0; // last button or touch
static uint32_t screensaverSince = 0, nextScreensaverStep = 0;
static int screensaverSteps = 0;

static int petCount() { int n = 0; for (auto &p : pets) n += p.used; return n; }
static int findPet(const char *id) {
  for (int i = 0; i < kMaxPets; ++i) if (pets[i].used && !strcmp(pets[i].id, id)) return i;
  return -1;
}
static int stepPet(int from, int dir) {
  for (int k = 1; k <= kMaxPets; ++k) {
    const int i = ((from + dir * k) % kMaxPets + kMaxPets) % kMaxPets;
    if (pets[i].used) return i;
  }
  return -1;
}
static bool needsYou(PetState s) { return s == PetState::Done || s == PetState::Error || s == PetState::Waiting; }
static bool busy(PetState s) { return s == PetState::Thinking || s == PetState::Tool; }

// Only a user page counts as looking; an automatic focus keeps `unseen`.
static void focusPet(int i, bool byUser) {
  focus = i;
  if (i < 0) return;
  if (byUser) pets[i].unseen = false;
  applyStyle(pets[i].style % kStyleCount, false);
  netOut.printf("GB_FOCUS %s\n", pets[i].id);
}

static int petState(const Pet &p, uint32_t now) {
  const uint32_t age = uint32_t(max<int32_t>(0, int32_t(now - p.since)));  // since may postdate this frame
  if (p.errorAt && int32_t(now - p.errorAt) < 2500 && p.state != PetState::Done) return SSurprised;
  switch (p.state) {
    case PetState::Thinking: return SThinking;
    case PetState::Tool: return SWorking;
    case PetState::Error: return SBlocked;
    case PetState::Waiting: return SWaiting;
    case PetState::Sleeping: return SSleeping;
    case PetState::Done:
      if (age < 4000) return SDone;
      if (p.unseen && age < 30000) return SWaiting;
      break;
    case PetState::Idle: break;
  }
  return age > 10 * 60000UL ? SSleeping : SIdle;
}

// "pet <id> <state> <style> <name>|<detail>"
static void handlePetLine(char *args) {
  char *id = strtok(args, " "), *st = strtok(nullptr, " "), *sty = strtok(nullptr, " "), *rest = strtok(nullptr, "");
  if (!id || !st || !sty) { netOut.println("GB_ERROR bad_pet"); return; }
  PetState ps = PetState::Idle;
  for (int k = 0; k < 7; ++k) if (!strcmp(st, kPetStateNames[k])) ps = PetState(k);
  const char *name = rest ? rest : "pi", *detail = "";
  if (rest) { char *bar = strchr(rest, '|'); if (bar) { *bar = 0; detail = bar + 1; } }
  const uint32_t now = millis();
  int i = findPet(id);
  const bool fresh = i < 0;
  if (fresh) {
    for (int k = 0; k < kMaxPets && i < 0; ++k) if (!pets[k].used) i = k;
    if (i < 0) { netOut.println("GB_ERROR pets_full"); return; }
    pets[i] = Pet{};
    pets[i].used = true;
    strncpy(pets[i].id, id, sizeof(pets[i].id) - 1);
    pets[i].state = ps;
    pets[i].since = now;
  }
  Pet &p = pets[i];
  const uint8_t style = uint8_t(atoi(sty)) % kStyleCount;
  const bool restyled = p.style != style;
  p.style = style;
  strncpy(p.name, name, sizeof(p.name) - 1);
  strncpy(p.detail, detail, sizeof(p.detail) - 1);
  p.detail[sizeof(p.detail) - 1] = 0;
  if (ps == PetState::Error) { p.errorAt = now ? now : 1; p.uhohDone = false; }
  // Transition sounds: a new session pops; a run starting ticks (not thinking <-> tool flips);
  // completion chimes; an error winces (tick() adds the uh-oh if it persists).
  const PetState prevState = p.state;
  auto resting = [](PetState s2) {
    return s2 == PetState::Idle || s2 == PetState::Done || s2 == PetState::Waiting || s2 == PetState::Sleeping;
  };
  int sound = -1;  // played after focus is settled: muting depends on the resulting roster
  if (fresh) sound = SndPop;
  else if (prevState != ps && ps == PetState::Done) sound = SndChime;
  else if (prevState != ps && ps == PetState::Error) sound = SndWince;
  else if (prevState != ps && busy(ps) && resting(prevState)) sound = SndStart;
  if (p.state != ps || fresh) {
    p.state = ps;
    p.since = now;
    if (needsYou(ps)) {
      p.unseen = true;
      if (focus != i && (focus < 0 || !busy(pets[focus].state))) focusPet(i, false);
    }
  }
  if (focus < 0) focusPet(i, false);
  else if (focus == i && restyled) applyStyle(style, false);
  if (sound >= 0) playSound(sound);
}

static void removePet(const char *id) {
  for (int i = 0; i < kMaxPets; ++i) {
    if (!pets[i].used || (strcmp(id, "*") && strcmp(pets[i].id, id))) continue;
    pets[i].used = false;
    if (focus == i) focusPet(stepPet(i, 1), false);
  }
  if (focus >= 0 && !pets[focus].used) focus = -1;
  if (focus < 0) {
    applyStyle(demoStyle, true);
    manualSleep = false;
    enterState(demoState, false);
  }
}

// ---------- motion ----------
static void motion(uint32_t now, float &lidT, float &eyeT) {
  const float r = now / 1000.0f, s = (now - stateSince) / 1000.0f;
  lidT = 1; eyeT = 1;
  switch (state) {
    case SIdle:
      d.t = 1.5f * sinf(.5f * r) + .6f * sinf(.17f * r); o.t = sinf(.27f * r);
      A.t = 1.2f * sinf(.85f * r); h.t = 1 + .007f * sinf(.85f * r);
      break;
    case SThinking:
      d.t = -9 + 5 * sinf(.35f * r); o.t = 5 * sinf(.3f * r); A.t = 2.5f * sinf(.6f * r); h.t = 1;
      break;
    case SWorking: {
      const float t = sinf(r * PI * 3.2f);
      d.t = 4 + 2.5f * t; o.t = 3; A.t = 1.5f + 3 * fmaxf(0, t); h.t = 1 - .02f * fmaxf(0, t);
      if (now >= nextSpecial) { startSpin(1, frand(-.25f, .25f), 6); nextSpecial = now + urand(6000, 9000); }
      break;
    }
    case SWaiting:  // web "bored": sagging, half-lidded, the occasional sigh
      d.t = -3 + 4 * sinf(.25f * r); o.t = 4 * sinf(.2f * r); A.t = 5 + 1.5f * sinf(.35f * r);
      h.t = .99f; lidT = .8f; eyeT = .98f;  // web: .6 raised to minLidOpen .8
      if (now >= nextSigh) { sighAt = now; nextSigh = now + urand(4000, 7000); }
      if (now - sighAt < 600) {
        const float t = (now - sighAt) / 600.0f;
        h.t = 1 + .05f * sinf(t * PI); A.t += 3 * sinf(t * PI);
      }
      break;
    case SBlocked:
      d.t = 0; o.t = 0; A.t = 0; h.t = 1;
      break;
    case SDone:  // web "celebrate"
      d.t = 0; o.t = 0; A.t = -2.5f * fabsf(sinf(1.6f * r)); h.t = 1; eyeT = 1.1f; lidT = 1.1f;
      break;
    case SSurprised: {
      const float e = fminf(s / 1.2f, 1);
      o.t = -4 * (1 - e); A.t = -8 * (1 - e); h.t = s < .2f ? 1.08f : 1; eyeT = 1.15f - .08f * e;
      d.t = 1.5f * sinf(11 * r) * (1 - e);
      break;
    }
    case SSleeping: {
      const float t = fminf(s / 2, 1), a = sinf(clampf(s / .5f, 0, 1) * PI);
      d.t = 4 * t + 2 * sinf(.25f * r); o.t = -2 * t; A.t = 8 * t + 3 * sinf(.55f * r) - 5 * a;
      h.t = 1 + .016f * sinf(.55f * r) + .05f * a;
      lidT = 1;  // web naturalLids: the closed-line expressions already read as shut
      break;
    }
    case SHappy: {
      const float e = sinf(2.4f * r);
      d.t = 3 * sinf(1.2f * r); o.t = 2.5f * sinf(1.1f * r); A.t = -3 * fabsf(e); h.t = 1 + .02f * e; eyeT = 1.05f;
      break;
    }
    case SCurious:
      d.t = 10 + 6 * sinf(.7f * r); o.t = 5 * sinf(.6f * r); A.t = -2 + 1.5f * sinf(.9f * r); h.t = 1.01f; eyeT = 1.08f;
      if (now >= nextTilt) { tiltPulseAt = now; nextTilt = now + urand(1600, 2800); }
      if (now - tiltPulseAt < 440) {
        const float t = sinf((now - tiltPulseAt) / 440.0f * PI);
        o.t += 8 * t; d.t += 5 * t;
      }
      break;
    case SExcited: {
      const float t = fmodf(2.2f * r, 1);
      A.t = -10 * sinf(t * PI) + 2; h.t = t < .1f ? .92f : t < .3f ? 1.05f : 1; o.t = 4 * sinf(1.1f * r);
      eyeT = 1.06f; d.t = 7 * sinf(r * PI * 2.2f);
      if (now >= nextSpecial) { startSpin(); nextSpecial = now + urand(2800, 5000); }
      break;
    }
    case SSad:
      d.t = 3 + 2 * sinf(.3f * r); o.t = 1.5f * sinf(.25f * r); A.t = 7 + sinf(.4f * r); h.t = .97f; lidT = .8f; eyeT = .97f;
      break;
  }
  if ((state == SHappy) && now >= nextSpecial) {  // web: happy/excited/proud occasionally spin or bounce
    if (frand(0, 1) < .55f) startSpin(); else { startSpin(); startBounce(); }
    nextSpecial = now + urand(9000, 18000);
  }
}

// Screensaver: with no sessions, drift through pleasant states (never Blocked or Sad) every 12 s
// and change head shape every third step. Buttons or touch pause it for a minute.
static constexpr int kScreensaverStates[] = {SIdle, SHappy, SCurious, SThinking, SWorking, SExcited, SWaiting, SDone, SSurprised};
static void updateScreensaver(uint32_t now) {
  if (focus >= 0 || manualSleep) { screensaverSince = 0; return; }
  if (!screensaverSince) { screensaverSince = now; nextScreensaverStep = now + 12000; }
  if (int32_t(now - lastInteractionMs) < 60000 && lastInteractionMs) { nextScreensaverStep = now + 12000; return; }
  if (int32_t(now - nextScreensaverStep) < 0) return;
  nextScreensaverStep = now + 12000;
  constexpr int n = sizeof(kScreensaverStates) / sizeof(kScreensaverStates[0]);
  int k = 0;
  while (k < n && kScreensaverStates[k] != state) ++k;
  demoState = kScreensaverStates[(k + 1) % n];
  enterState(demoState, false);
  if (++screensaverSteps % 3 == 0) { demoStyle = (demoStyle + 1) % kStyleCount; applyStyle(demoStyle, false); }
}

// Brightness: full after any interaction; the screensaver dims after a minute and more after ten;
// a quiet roster (no pet working or needing you for 5 min) dims; sleeping is dimmest.
static void updateBrightness(uint32_t now) {
  static int current = -1;
  static uint32_t quietSince = 0;
  const uint32_t sinceTouch = lastInteractionMs ? now - lastInteractionMs : now;
  int target = 255;
  if (state == SSleeping) target = 70;
  else if (focus < 0) {
    const uint32_t sinceSaver = screensaverSince ? now - screensaverSince : 0;
    const uint32_t quiet = min(sinceTouch, sinceSaver);
    target = quiet < 60000 ? 255 : quiet < 600000 ? 120 : 50;
  } else {
    bool busyOrNeeds = false;
    for (auto &p : pets) if (p.used && (busy(p.state) || needsYou(p.state) || p.unseen)) busyOrNeeds = true;
    if (busyOrNeeds || !quietSince) quietSince = busyOrNeeds ? 0 : now;
    target = (!busyOrNeeds && quietSince && now - quietSince > 300000 && sinceTouch > 300000) ? 120 : 255;
  }
  if (sinceTouch < 60000 && state != SSleeping) target = 255;
  if (target != current) { M5.Display.setBrightness(target); current = target; }
}

static void tick(float dt, uint32_t now) {
  // The hub pings every 5 s; 20 s of silence means it quit or the link is gone: clear the stale
  // roster (the hub resyncs everything when it reconnects) and fall back to the screensaver.
  if (petCount() && int32_t(now - lastHubMs) > 20000) {
    removePet("*");
    netOut.println("GB_EVENT hub_gone");
  }
  updateScreensaver(now);
  updateBrightness(now);
  { const float target = focus >= 0 ? kHomeYBand : kHomeYCentred;  // ease ~0.3 s
    kHomeY += (target - kHomeY) * (1 - expf(-dt / .12f)); }
  for (auto &p : pets) {  // still errored when the 2.5 s wince ends: blocked -> "uh-oh"
    // Signed difference: serial input can stamp errorAt after this frame's `now`.
    if (p.used && p.state == PetState::Error && p.errorAt && !p.uhohDone && int32_t(now - p.errorAt) >= 2500) {
      p.uhohDone = true;
      playSound(SndUhoh);
    }
  }
  if (focus >= 0) {
    const int want = manualSleep ? SSleeping : petState(pets[focus], now);
    if (want != state) enterState(want, false);
    badge = pets[focus].unseen;
  } else {
    badge = false;
  }
  const StateDef &def = kStates[state];

  // Expressions, blinks, gaze schedule.
  if (now >= nextExpr) {
    const int next = def.expr[def.nExpr > 1 ? (esp_random() % def.nExpr) : 0];
    changeExpr(next, def.exprSpeed, now);
    nextExpr = now + urand(def.exprMin, def.exprMax);
  }
  if (pendingExpr >= 0 && now >= pendingAt) { setExpr(pendingExpr, 16); pendingExpr = -1; }
  if (def.blinkMax && now >= nextBlink) { blinkNow(now); nextBlink = now + urand(def.blinkMin, def.blinkMax); }
  if (def.winks && now >= nextWink) { winkAt = now; winkEye = esp_random() & 1; nextWink = now + urand(4500, 10000); }
  if (now >= nextGaze && now >= gazeHoldUntil) {
    const float sx = (def.gxSigned ? (frand(0, 1) < .5f ? -1 : 1) : 1) * frand(def.gx0, def.gx1) * 15;
    gazeX.t = sx; gazeY.t = frand(def.gy0, def.gy1) * 9;
    nextGaze = now + urand(def.gazeMin, def.gazeMax);
  }
  if (touchDown && !dragging) {  // look at the finger
    const float dx = touchX - (kHomeX + dragX.x), dy = touchY - (kHomeY + dragY.x);
    const float len = hypotf(dx, dy), m = clampf(len / 150, 0, 1) / fmaxf(len, 1);
    gazeX.t = dx * m * 15; gazeY.t = dy * m * 9;
  }

  float lidT, eyeT;
  motion(now, lidT, eyeT);
  eyeScale.t = eyeT;
  // Lid follows blink keyframes while any remain, otherwise the state's openness.
  float key = -1;
  while (lidKeys && now >= lidKeyAt[0]) {
    key = lidKeyV[0];
    for (int i = 1; i < lidKeys; ++i) { lidKeyAt[i - 1] = lidKeyAt[i]; lidKeyV[i - 1] = lidKeyV[i]; }
    --lidKeys;
  }
  if (key >= 0) lid.t = key;
  else if (!lidKeys) lid.t = lidT;

  // Glyph entry starts at once in the reference; a 350 ms debounce on Thinking (adaptation) keeps
  // sub-second tool bursts from flickering the face.
  const bool glyphOn = def.glyph != GNone && (def.glyph != GDots || now - stateSince > 350);
  glyphMix.t = glyphOn ? 1 : 0;
  if (def.glyph != GNone) styleMix.t = def.glyph == GBang ? 1 : 0;  // exit keeps the outgoing glyph
  if (glyphMix.x < .02f && glyphOn) { styleMix.x = styleMix.t; styleMix.v = 0; }
  if (glyphOn != glyphPresent) {  // half-turn in on entry, the second half-turn (same way) on exit
    if (glyphOn) eyeTurnDir = frand(0, 1) < .5f ? 1 : -1;
    eyeTurn.t += PI * eyeTurnDir;
    glyphPresent = glyphOn;
  }

  // Drag / tilt.
  if (dragging) {
    dragX.t = clampf(touchX - grabDX - kHomeX, -140, 140);
    dragY.t = clampf(touchY - grabDY - kHomeY, -120, 110);
  } else {
    dragX.t = clampf(tiltX, -1, 1) * 40; dragY.t = clampf(tiltY, -1, 1) * 30;
  }
  sqx.t = 1; sqy.t = 1;

  // Fixed 1/120 s substeps with the web springs' constants.
  const int n = max(1, int(ceilf(dt * 120)));
  const float step = dt / n;
  for (int i = 0; i < n; ++i) {
    blend.step(exprSpeed, 1, step);
    d.step(5, .9f, step); o.step(3.5f, 1, step); A.step(4, 1, step); h.step(10, .8f, step);
    lid.step(26, 1, step); eyeScale.step(9, .85f, step);
    gazeX.step(13, 1, step); gazeY.step(13, 1, step);
    glyphMix.step(14, 1, step); headMix.step(10, 1, step);
    styleMix.step(11, 1, step); eyeTurn.step(14, 1, step);
    if (spinning) spin.step(6.2f, 1, step);
    inkR.step(9, 1, step); inkG.step(9, 1, step); inkB.step(9, 1, step);
    sqx.step(16, .35f, step); sqy.step(16, .35f, step);
    dragX.step(dragging ? 18 : 7, dragging ? 1 : .6f, step); dragY.step(dragging ? 18 : 7, dragging ? 1 : .6f, step);
  }
  if (!glyphPresent && fabsf(eyeTurn.t - eyeTurn.x) < .004f && fabsf(eyeTurn.v) < .015f &&
      fabsf(fmodf(eyeTurn.t, 2 * PI)) < .01f) eyeTurn.init(0);  // full turn completed
  if (spinning && fabsf(spin.t - spin.x) < .004f && fabsf(spin.v) < .015f) {
    spinning = false; spin.init(0);
    if (spinQueue > 0 && state == SDone) { --spinQueue; startSpin(frand(1, 2), frand(-1.1f, 1.1f), 4); }
  }
  { const float tnow = now / 1000.0f;  // retire ribbons
    for (int i = 0; i < ribbonCount;) { if (tnow - ribbons[i].born > ribbons[i].life) ribbons[i] = ribbons[--ribbonCount]; else ++i; } }

  for (int i = 0; i < partCount;) {  // confetti
    Particle &p = parts[i];
    p.vy += 900 * dt; p.vx *= 1 - 1.2f * dt; p.x += p.vx * dt; p.y += p.vy * dt; p.rot += p.vr * dt; p.life += dt;
    if (p.life > 1.8f || p.y > H + 20) parts[i] = parts[--partCount]; else ++i;
  }
}

// ---------- rendering ----------
static inline uint16_t pack(float r, float g, float b) {
  uint16_t c = (uint16_t(clampf(r, 0, 255)) >> 3) << 11 | (uint16_t(clampf(g, 0, 255)) >> 2) << 5 |
               (uint16_t(clampf(b, 0, 255)) >> 3);
  return uint16_t((c >> 8) | (c << 8));  // sprite buffer is byte-swapped RGB565
}
static inline uint16_t unswap(uint16_t c) { return uint16_t((c >> 8) | (c << 8)); }

// Bilinear sample of a head SDF texture at head-unit coordinates.
static inline float sampleSdf(const int8_t *tex, float u, float v) {
  constexpr float inv = 1.0f / kSdfStep;
  float fx = (u - kSdfLo) * inv, fy = (v - kSdfLo) * inv;
  if (fx < 0 || fy < 0 || fx >= kSdfN - 1 || fy >= kSdfN - 1) return 31.75f;
  const int ix = int(fx), iy = int(fy);
  const float tx = fx - ix, ty = fy - iy;
  const int8_t *p = tex + iy * kSdfN + ix;
  const float a = p[0] + (p[1] - p[0]) * tx, b = p[kSdfN] + (p[kSdfN + 1] - p[kSdfN]) * tx;
  return (a + (b - a) * ty) / kSdfQuant;
}

static inline float sdCapsule(float px, float py, const Cap &c) {
  const float qx = px - c.cx, qy = py - c.cy;
  const float s = sinf(c.ang), co = cosf(c.ang);
  const float along = qx * s + qy * co, perp = qx * co - qy * s;
  const float a = fabsf(along) - c.len;
  const float ya = a > 0 ? a : 0;
  return sqrtf(perp * perp + ya * ya) - c.wid;
}

// Per-frame eye geometry, head-local units relative to the centre (0, 0).
struct EyeFrame {
  bool visible;
  float cx, cy, len, wid, sinA, cosA;  // capsule
  bool roundLid;                       // round eyes close vertically (as a flattening capsule)
  float xs, ixs;                       // spin foreshortening (along the spin direction) and inverse
  float sa, ca;                        // spin axis rotation
  bool poly;                           // settled expression: true contour (SDF texture)
  float K, lax, lay, lg, linv;         // contour scale; lid axis, squash g = 1 - lid, g / (1 - g)
  float bx, by;                        // squashed contour half extents (bounding box about the centre)
};

static Rect prevRect = {0, 0, W, H};
static bool fullRedraw = true;
static constexpr Rect kBand = {100, 372, 368, 450};
static bool bandOn = false;
static uint32_t profFill = 0, profTiles = 0, profExtra = 0, profShaded = 0, profPoly = 0;

static void drawBand(uint32_t now, RGB bg) {
  canvas.fillRect(kBand.x0, kBand.y0, kBand.x1 - kBand.x0, kBand.y1 - kBand.y0, unswap(pack(bg.r, bg.g, bg.b)));
  const Pet &p = pets[focus];
  const RGB l = rgb(kLabel);
  canvas.setTextDatum(textdatum_t::middle_center);
  canvas.setTextSize(1);
  canvas.setFont(&fonts::FreeSansBold12pt7b);
  canvas.setTextColor(unswap(pack(l.r, l.g, l.b)));
  canvas.drawString(p.name, W / 2, 390);
  char detail[44];
  snprintf(detail, sizeof(detail), "%s", p.detail[0] ? p.detail : kPetStateNames[int(p.state)]);
  canvas.setFont(&fonts::FreeSans9pt7b);
  while (canvas.textWidth(detail) > 236 && strlen(detail) > 4) strcpy(detail + strlen(detail) - 4, "...");
  canvas.setTextColor(unswap(pack(mixf(bg.r, l.r, 0.7f), mixf(bg.g, l.g, 0.7f), mixf(bg.b, l.b, 0.7f))));
  canvas.drawString(detail, W / 2, 417);
  const int n = petCount();
  if (n < 1) return;
  const uint16_t bg565ForBand = pack(bg.r, bg.g, bg.b);
  const float pulse = 0.5f + 0.5f * sinf(now / 1000.0f * 2 * PI * 1.2f);
  int k = 0;
  for (int i = 0; i < kMaxPets; ++i) {
    if (!pets[i].used) continue;
    const float x = W / 2 + (k++ - (n - 1) / 2.0f) * 18.0f, y = 438;
    // Fill = the session's identity colour; ring = its state.
    //   working: amber · thinking: soft white · error/blocked: red · finished/waiting and unseen:
    //   pulsing blue · idle/seen: none. Sleeping pets dim.
    const Pet &pp = pets[i];
    RGB c = rgb(kStyles[pp.style % kStyleCount].ink);
    const bool asleep = pp.state == PetState::Sleeping || (i == focus && manualSleep);
    if (asleep) c = {c.r * .4f, c.g * .4f, c.b * .4f};
    int ring = -1;  // hex colour
    bool pulsing = false;
    if (pp.state == PetState::Error) { ring = 0xFF4D4F; pulsing = pp.unseen; }
    else if (pp.state == PetState::Tool) ring = 0xF5B13F;
    else if (pp.state == PetState::Thinking) ring = 0xD8D4CA;
    else if (pp.unseen) { ring = 0x1D9BF0; pulsing = true; }
    const float rf = i == focus ? 5 : 3.5f, ro = rf + 2.8f;
    if (ring >= 0) {
      const RGB rc = rgb(uint32_t(ring));
      const float a = pulsing ? .35f + .65f * pulse : 1;
      canvas.fillSmoothCircle(x, y, ro, unswap(pack(mixf(bg.r, rc.r, a), mixf(bg.g, rc.g, a), mixf(bg.b, rc.b, a))));
      canvas.fillSmoothCircle(x, y, rf + 1.2f, unswap(bg565ForBand));
    }
    canvas.fillSmoothCircle(x, y, rf, unswap(pack(c.r, c.g, c.b)));
  }
}

static Rect render(uint32_t now) {
  const RGB bg = rgb(kBg), ink = {inkR.x, inkG.x, inkB.x};
  const uint16_t bg565 = pack(bg.r, bg.g, bg.b), ink565 = pack(ink.r, ink.g, ink.b);
  const bool labelOn = now < labelUntil;
  const bool full = fullRedraw || labelOn || state == SSleeping || partCount > 0;

  // Head transform: translate, rotate (deg), vertical scale; plus touch squash.
  float bounce = 0;
  if (bounceStart >= 0) {  // web bounce chain: decaying parabolic hops
    static const float kH[4] = {48, 28, 14, 6}, kD[4] = {.5f, .382f, .27f, .177f};
    float t = (now - uint32_t(bounceStart)) / 1000.0f;
    int i = 0;
    while (i < 4 && t >= kD[i]) t -= kD[i++];
    if (i >= 4) bounceStart = -1;
    else { const float s = t / kD[i]; bounce = -4 * kH[i] * s * (1 - s); }
  }
  // Glyph weights: the head is retained and contracts into the glyph; body motion decays with 1 - g.
  const float gm = clampf(glyphMix.x, 0, 1), sm = clampf(styleMix.x, 0, 1);
  const float bangW = gm * sm, dotsW = gm - bangW, residual = 1 - gm;
  const float sxS = mixf(clampf(sqx.x, .7f, 1.4f), 1, gm), syS = mixf(clampf(sqy.x * h.x, .7f, 1.4f), 1, gm);
  const float hcx = kHomeX + dragX.x + o.x * kScale * residual;
  const float hcy = kHomeY + dragY.x + (A.x + bounce) * kScale * residual;
  const float rot = d.x * residual * PI / 180, cr = cosf(rot), sr = sinf(rot);
  const float ksx = 1 / (kScale * sxS), ksy = 1 / (kScale * syS), dpx = kScale * fminf(sxS, syS);
  const float hm = clampf(headMix.x, 0, 1);
  // ---- Glyph geometry (reference formulas; head units, centred) ----
  auto easeOut = [](float x) { const float t = 1 - x; return 1 - t * t * t; };
  auto easeBack = [](float x) { const float t = x - 1; return 1 + 2.70158f * t * t * t + 1.70158f * t * t; };
  auto easeInOut = [](float x) { return x < .5f ? 4 * x * x * x : 1 - powf(-2 * x + 2, 3) / 2; };
  float pLift[3], pPop[3], pTone[3];  // travelling Gaussian pulse: size, lift, and brightness together
  {
    const float phase = fmodf(fmodf((now - dotsPhase0) / 1400.0f + .119f, 1) + 1, 1);
    for (int j = 0; j < 3; ++j) {
      float dd = fabsf(phase - j / 3.0f);
      dd = fminf(dd, 1 - dd);
      const float sp = expf(-dd * dd / .045f);
      pLift[j] = 9 * sp * (j == 1 ? gm : dotsW); pPop[j] = .84f + .22f * sp; pTone[j] = .5f + .5f * sp;
    }
  }
  const float bt = (now - bangT0) / 1000.0f, bDecay = expf(-5.5f * fmodf(bt, 2.2f));
  // Retained head -> Thinking's centre dot (r 22) or Blocked's lower dot (r 13, dropping to +58).
  const float corePop = (1 + (pPop[1] - 1) * (dotsW / fmaxf(gm, .001f))) * (1 + .04f * bDecay * bangW);
  const float coreScale = residual + mixf(22, 13, sm) / C * corePop * gm;
  const float coreY = -pLift[1] * dotsW + 58 * bangW * gm;
  const float roundW = easeInOut(clampf(gm / .62f, 0, 1));  // rounding completes by 62 % of the morph
  const float coreTone = 1 - (1 - pTone[1]) * dotsW;
  // Satellites: staggered, overshooting spread.
  float satX[2], satY[2], satR[2], satTone[2];
  for (int side = 0; side < 2; ++side) {
    const float u = clampf((dotsW - .12f * side) / (1 - .12f * side), 0, 1);
    const int j = side == 0 ? 0 : 2;
    const float grow = easeOut(u);
    satX[side] = (side == 0 ? -62 : 62) * easeBack(u); satY[side] = -pLift[j];
    satR[side] = u > .004f ? 22 * grow * pPop[j] * 1.02f : 0; satTone[side] = grow * pTone[j];
  }
  // Tapered stem: drops in, grows, fades in, and shakes about its cap (decaying every 2.2 s).
  const float arrive = easeOut(clampf(1.1f * bangW, 0, 1)), stemScale = clampf(1.2f * bangW, 0, 1);
  const float stemTone = clampf(1.5f * bangW - .2f, 0, 1), stemDrop = -26 - 70 * (1 - arrive);
  const float shake = 2.2f * sinf(42 * bt) * bDecay * PI / 180, shC = cosf(shake), shS = sinf(shake);
  constexpr float kPivotY = 40.3f - C;
  auto stemSdf = [&](float u, float v) -> float {
    if (stemScale < .01f) return 1e9f;
    float x = u, y = v - stemDrop - kPivotY;           // undo drop, move pivot to origin
    const float xr = x * shC + y * shS, yr = -x * shS + y * shC;  // undo rotation
    x = xr / stemScale; y = (yr + kPivotY) / stemScale;  // undo scale about the head centre
    // Source path tH: cap r 15 at y -33, foot r 8.5 at y +39.5 (iq uneven capsule).
    const float px = fabsf(x), py = y + 33, r1 = 15, r2 = 8.5f, hh = 72.5f;
    const float b = (r1 - r2) / hh, a = fsqrt(1 - b * b), k = -b * px + a * py;
    float dd;
    if (k < 0) dd = fsqrt(px * px + py * py) - r1;
    else if (k > a * hh) dd = fsqrt(px * px + (py - hh) * (py - hh)) - r2;
    else dd = a * px + b * py - r1;
    return dd * stemScale;
  };
  auto headAt = [&](float u, float v) -> float {
    float dh = sampleSdf(sdfCur, u + C, v + C);
    if (hm < .999f) dh = mixf(sampleSdf(sdfPrev, u + C, v + C), dh, hm);
    return dh;
  };
  // Each glyph part (retained core, two satellites, stem) with its own opacity; parts composite
  // "over" one another like the reference's separately drawn shapes.
  const float partA[4] = {coreTone, satTone[0], satTone[1], stemTone};
  auto glyphParts = [&](float u, float v, float *d) -> float {
    const float qx = u / coreScale, qy = (v - coreY) / coreScale;
    float dcore;
    if (roundW >= .999f) dcore = fsqrt(qx * qx + qy * qy) - C;
    else dcore = mixf(headAt(qx, qy), fsqrt(qx * qx + qy * qy) - C, roundW);
    d[0] = dcore * coreScale; d[1] = d[2] = 1e9f; d[3] = stemSdf(u, v);
    for (int side = 0; side < 2; ++side) {
      if (satR[side] <= .3f) continue;
      const float du = u - satX[side], dv = v - satY[side];
      d[1 + side] = fsqrt(du * du + dv * dv) - satR[side];
    }
    return fminf(fminf(d[0], d[1]), fminf(d[2], d[3]));
  };
  auto bodySdf = [&](float u, float v) -> float {  // u, v centred head units
    if (gm <= .001f) return headAt(u, v);
    float d[4];
    return glyphParts(u, v, d);
  };

  // Eyes.
  const float *face = kHeadFace[headCur];
  EyeFrame eyes[2];
  if (blend.x >= .999f) loadEyeTex(exprIndex);
  const bool eyesOn = gm < .5f;
  const float morphPop = 1 + .07f * sinf(clampf(blend.x, 0, 1) * PI);
  // Cap the eye scale so the two eyes never merge (web: eB from spacing vs. half widths).
  const Cap c0 = blendedCap(0), c1 = blendedCap(1);
  auto halfX = [](const Cap &c) { return fabsf(sinf(c.ang)) * c.len + c.wid; };
  const float spacing = fabsf(c1.cx - c0.cx) * face[2];
  const float capScale = halfX(c0) + halfX(c1) > .5f ? clampf((spacing - 5) / (halfX(c0) + halfX(c1)), .35f, 4) : 4;
  const float K = fminf(face[4] * clampf(eyeScale.x, .2f, 2), capScale / morphPop) * morphPop;
  for (int i = 0; i < 2; ++i) {
    EyeFrame &e = eyes[i];
    const Cap c = blendedCap(i);
    e.visible = eyesOn;
    float ex = face[0] + (c.cx - C) * face[2] + gazeX.x * face[2];
    float ey = face[1] + (c.cy - C) * face[3] + gazeY.x * face[3];
    ex += 1.4f * sinf(.00042f * now + i) + .5f * sinf(.001f * now + 2 * i);  // idle drift
    ey += .9f * sinf(.00058f * now + i);
    e.len = c.len * K; e.wid = c.wid * K;
    e.sinA = sinf(c.ang); e.cosA = cosf(c.ang);
    float l = fmaxf(lid.x, .04f);
    if (i == winkEye && now - winkAt < 320) {
      const float t = (now - winkAt) / 320.0f;
      l = fmaxf(l * (t < .42f ? 1 - t / .42f : (t - .42f) / .58f), .04f);
    }
    l = clampf(l, .04f, 1.2f);
    // Settled expressions draw the true contour, squashed along the web's lid axis; mid-morph the
    // web itself interpolates capsule fits, which the capsule path below reproduces.
    e.poly = blend.x >= .999f;
    float hx, hy;
    if (e.poly) {
      const float *lax = kEyeLid[exprIndex][i];
      const float g = 1 - l;
      e.K = K; e.lax = lax[0]; e.lay = lax[1]; e.lg = g > .001f ? g : 0; e.linv = e.lg / fmaxf(1 - e.lg, .01f);
      hx = hy = 0;
      for (int k = 0; k < kEyePts; ++k) {  // squashed extents (web applies the lid to the points)
        float dx = kEyeContour[exprIndex][i][k][0] - lax[2], dy = kEyeContour[exprIndex][i][k][1] - lax[3];
        const float a = dx * lax[0] + dy * lax[1];
        dx = (dx - a * g * lax[0]) * K; dy = (dy - a * g * lax[1]) * K;
        hx = fmaxf(hx, fabsf(dx)); hy = fmaxf(hy, fabsf(dy));
      }
      e.len = 0; e.wid = fmaxf(hx, hy);  // bounding radius for tile culling
      e.bx = hx; e.by = hy;
    } else {
      hx = fabsf(e.sinA) * e.len + e.wid; hy = fabsf(e.cosA) * e.len + e.wid;
    }
    // Keep the whole eye inside the head (web clamps each eye's extent to the head span): test the
    // four extremes and push the centre inward by any overshoot.
    {
      hx += 2; hy += 2;
      for (int k = 0; k < 3; ++k) {
        float mx = 0, my = 0;
        const float dl = sampleSdf(sdfCur, ex - hx + C, ey + C), dr = sampleSdf(sdfCur, ex + hx + C, ey + C);
        const float dt = sampleSdf(sdfCur, ex + C, ey - hy + C), db = sampleSdf(sdfCur, ex + C, ey + hy + C);
        if (dl > 0) mx += dl; if (dr > 0) mx -= dr;
        if (dt > 0) my += dt; if (db > 0) my -= db;
        if (mx == 0 && my == 0) break;
        ex += mx; ey += my;
      }
    }
    // Spin: the eyes ride a cylinder about the spin axis (tilted from vertical by spinAxis).
    e.xs = 1; e.sa = sinf(spinAxis); e.ca = cosf(spinAxis);
    if (spinning || fabsf(eyeTurn.x) > .001f) {
      if (!spinning) { e.sa = 0; e.ca = 1; }  // glyph half-turns rotate about the vertical axis
      const float turn = (spinning ? spin.x : 0) + eyeTurn.x;
      const float R = 100 * face[2];
      const float a = ex * e.ca + ey * e.sa, b = -ex * e.sa + ey * e.ca;  // a: across the axis
      const float th0 = asinf(clampf(a / R, -1, 1)), th = th0 + turn;
      const float cth = cosf(th);
      e.visible = e.visible && cth > .02f;
      e.xs = clampf(fmaxf(cth, .02f) / fmaxf(cosf(th0), .02f), .02f, 1.3f);
      const float a2 = R * sinf(th);
      ex = a2 * e.ca - b * e.sa; ey = a2 * e.sa + b * e.ca;
    }
    e.cx = ex; e.cy = ey; e.ixs = 1 / e.xs;
    if (e.poly) continue;
    // Blink along the long axis; round eyes flatten vertically. Shrink the capsule itself so the
    // distance field stays exact (crisp edges at every lid position).
    // The web squashes the eye polygon along its long axis (round eyes: vertically), thickness
    // included, so a closing pill becomes a flat dash. As a capsule: shorten along the axis; once
    // shorter than it is wide, it is a dash lying across the old axis.
    const float elong = c.len / fmaxf(c.len + c.wid, .01f);
    e.roundLid = elong < .15f;
    if (e.roundLid) { e.sinA = 0; e.cosA = 1; }  // squash axis vertical
    const float half = e.len + e.wid, tot = half * l;
    if (tot >= e.wid) {
      e.len = tot - e.wid;
    } else {
      const float nw = fmaxf(tot, 1.4f);
      e.len = fmaxf(e.wid - nw, 0); e.wid = nw;
      const float s0 = e.sinA; e.sinA = e.cosA; e.cosA = -s0;  // rotate the axis 90 degrees
    }
  }
  // Eyes live inside the retained head's transform, so they contract with it into the glyph.
  const float invCore = 1 / coreScale;
  auto eyeOne = [&](int i, float u, float v) -> float {
    const EyeFrame &e = eyes[i];
    if (!e.visible) return 1e9f;
    float qx = u * invCore - e.cx, qy = (v - coreY) * invCore - e.cy;
    if (e.xs != 1) {  // foreshorten across the spin axis
      const float a = (qx * e.ca + qy * e.sa) * e.ixs, b = -qx * e.sa + qy * e.ca;
      qx = a * e.ca - b * e.sa; qy = a * e.sa + b * e.ca;
    }
    if (e.poly) {  // exact signed distance to the 48-point contour (iq polygon SDF)
      const float scale = fminf(e.xs, 1) * coreScale;
      // Distance to the bounding box is a lower bound: far from the eye, skip the contour.
      const float ox = fabsf(qx) - e.bx, oy = fabsf(qy) - e.by;
      if (ox > 3 || oy > 3) {
        const float ax = fmaxf(ox, 0), ay = fmaxf(oy, 0);
        return fsqrt(ax * ax + ay * ay) * scale;
      }
      ++profPoly;
      // Undo the lid squash exactly (a one-axis scale by 1 - g), sample the contour's distance
      // texture, and correct the distance by the squash's effect on the local gradient.
      const float iK = 1 / e.K;
      float cx = qx * iK, cy = qy * iK;
      if (e.lg > 0) { const float sp = cx * e.lax + cy * e.lay; cx += e.linv * sp * e.lax; cy += e.linv * sp * e.lay; }
      float dd = sampleEyeTex(i, cx, cy);
      if (e.lg > 0) {
        if (fabsf(dd) < 6) {
          const float gxv = sampleEyeTex(i, cx + .5f, cy) - sampleEyeTex(i, cx - .5f, cy);
          const float gyv = sampleEyeTex(i, cx, cy + .5f) - sampleEyeTex(i, cx, cy - .5f);
          const float gl = fsqrt(gxv * gxv + gyv * gyv);
          const float an = gl > 1e-4f ? (gxv * e.lax + gyv * e.lay) / gl : 0;
          const float k1 = 1 / (1 - e.lg);
          dd /= fsqrt(1 + (k1 * k1 - 1) * an * an);
        } else {
          dd *= 1 - e.lg;  // conservative lower bound far from the edge
        }
      }
      return dd * e.K * scale;
    }
    const float along = qx * e.sinA + qy * e.cosA, perp = qx * e.cosA - qy * e.sinA;
    const float a = fabsf(along) - e.len, ya = a > 0 ? a : 0;
    return (fsqrt(perp * perp + ya * ya) - e.wid) * fminf(e.xs, 1) * coreScale;
  };
  // mask: bit 0 = left eye, bit 1 = right eye
  auto eyeSdf = [&](float u, float v, int mask = 3) -> float {
    float m = 1e9f;
    if (mask & 1) m = eyeOne(0, u, v);
    if (mask & 2) m = fminf(m, eyeOne(1, u, v));
    return m;
  };

  // Badge: a blue dot on the upper right when the pet needs you.
  const float badgeR = 13, ringR = badgeR + 5;
  const float bu = 78, bv = -80;  // head units
  const float bsx = hcx + (bu * cr - bv * sr) * kScale * sxS, bsy = hcy + (bu * sr + bv * cr) * kScale * syS;
  const float bScreenR = badgeR * kScale, bRingR = ringR * kScale;
  const bool showBadge = badge && gm < .5f;

  // Screen bounds of the body (conservative: rotated square of the reach).
  const float reach = (ribbonCount ? 156 : 124) * kScale * fmaxf(sxS, syS) + (gm > .01f ? 20 : 0);
  Rect r = {int(hcx - reach), int(hcy - reach - 10), int(hcx + reach) + 1, int(hcy + reach) + 1};
  r.x0 = max(r.x0, 0); r.y0 = max(r.y0, 0); r.x1 = min(r.x1, W); r.y1 = min(r.y1, H);
  Rect dirty = full ? Rect{0, 0, W, H}
                    : Rect{min(r.x0, prevRect.x0), min(r.y0, prevRect.y0), max(r.x1, prevRect.x1), max(r.y1, prevRect.y1)};
  uint32_t pt0 = micros();
  canvas.fillRect(dirty.x0, dirty.y0, dirty.x1 - dirty.x0, dirty.y1 - dirty.y0, unswap(bg565));
  profFill += micros() - pt0; pt0 = micros();

  // Orbit ribbons: tilted circles around the head; the back half is drawn now (the body covers
  // it), the front half after the body.
  auto drawRibbons = [&](bool front) {
    const float tnow = now / 1000.0f;
    for (int i = 0; i < ribbonCount; ++i) {
      const Ribbon &rb = ribbons[i];
      const float age = tnow - rb.born;
      if (age < 0) continue;
      const float alpha = clampf(age / .25f, 0, 1) * clampf((rb.life - age) / .5f, 0, 1);
      const float head = rb.phase + rb.speed * age;
      const float ct = cosf(rb.tilt), st = sinf(rb.tilt), cr2 = cosf(rb.roll), sr2 = sinf(rb.roll);
      const RGB c1 = rgb(rb.color), c2 = rgb(kConfetti[(i + 2) % 6]);
      constexpr int kSeg = 12;
      float px = 0, py = 0; bool pfront = false;
      for (int k = 0; k <= kSeg; ++k) {
        const float f = float(k) / kSeg;
        const float th = head - rb.len * (1 - f) * (rb.speed > 0 ? 1 : -1);
        const float x = rb.radius * cosf(th), y1 = rb.radius * sinf(th);
        const float y = y1 * ct, z = y1 * st;
        const float X = x * cr2 - y * sr2, Y = x * sr2 + y * cr2;
        const float sx = hcx + X * kScale, sy = hcy + Y * kScale;
        const bool isFront = z > 0;
        if (k > 0 && isFront == front && pfront == front) {
          const float w = (2.0f + 7.0f * sinf(f * PI)) * alpha * kScale;
          const RGB c = {mixf(c1.r, c2.r, f), mixf(c1.g, c2.g, f), mixf(c1.b, c2.b, f)};
          // Thick segment as a quad (two triangles): far cheaper than antialiased wide lines.
          const float dx = sx - px, dy = sy - py, dl = fmaxf(fsqrt(dx * dx + dy * dy), .01f);
          const float nx = -dy / dl * w * .5f, ny = dx / dl * w * .5f;
          const uint16_t col = unswap(pack(c.r, c.g, c.b));  // fades by width, never toward the background
          canvas.fillTriangle(int(px + nx), int(py + ny), int(px - nx), int(py - ny), int(sx + nx), int(sy + ny), col);
          canvas.fillTriangle(int(px - nx), int(py - ny), int(sx - nx), int(sy - ny), int(sx + nx), int(sy + ny), col);
        }
        px = sx; py = sy; pfront = isFront;
      }
    }
  };
  if (ribbonCount) drawRibbons(false);

  auto toLocal = [&](float x, float y, float &u, float &v) {
    const float dx = x - hcx, dy = y - hcy;
    u = (dx * cr + dy * sr) * ksx; v = (-dx * sr + dy * cr) * ksy;
  };
  // Eye screen box.
  float eyeX0 = 1e9f, eyeX1 = -1e9f, eyeY0 = 1e9f, eyeY1 = -1e9f;
  for (auto &e : eyes) {
    if (!e.visible) continue;
    const float ecx = e.cx * coreScale, ecy = e.cy * coreScale + coreY;
    const float sx = hcx + (ecx * cr - ecy * sr) * kScale * sxS, sy = hcy + (ecx * sr + ecy * cr) * kScale * syS;
    const float rr = (e.len + e.wid) * coreScale * kScale * 1.4f + 4;
    eyeX0 = fminf(eyeX0, sx - rr); eyeX1 = fmaxf(eyeX1, sx + rr); eyeY0 = fminf(eyeY0, sy - rr); eyeY1 = fmaxf(eyeY1, sy + rr);
  }

  auto shade = [&](int x, int y, int bodyMode, int eyeMask, bool doBadge) -> int {
    float u, v;
    toLocal(x + .5f, y + .5f, u, v);
    float cov = bodyMode == 2 ? 1 : 0, glyphK = -1;
    if (gm > .001f) {  // composite glyph parts; eyes cut only the retained core
      float d[4];
      glyphParts(u, v, d);
      const float ec = eyeMask ? clampf(.5f - eyeSdf(u, v, eyeMask) * dpx, 0, 1) : 0;
      float keep = 1;
      for (int i = 0; i < 4; ++i) {
        if (d[i] > 1e8f) continue;
        float c = clampf(.5f - d[i] * dpx, 0, 1) * partA[i];
        if (i == 0) c *= 1 - ec;
        keep *= 1 - c;
      }
      glyphK = 1 - keep;
      cov = glyphK > 0 ? 1 : 0;
    } else if (bodyMode == 1) cov = clampf(.5f - bodySdf(u, v) * dpx, 0, 1);
    float ringCov = 0, dd = 0;
    if (doBadge) {
      const float bx = x + .5f - bsx, by = y + .5f - bsy;
      dd = fsqrt(bx * bx + by * by);
      ringCov = clampf(bRingR + .5f - dd, 0, 1);
    }
    if (cov <= 0 && ringCov <= 0) return -1;
    float cr_ = bg.r, cg = bg.g, cb = bg.b;
    if (cov > 0) {
      float k;
      if (glyphK >= 0) k = glyphK;
      else {
        const float ec = eyeMask ? clampf(.5f - eyeSdf(u, v, eyeMask) * dpx, 0, 1) : 0;  // eyes are holes
        k = cov * (1 - ec);
      }
      cr_ = mixf(bg.r, ink.r, k); cg = mixf(bg.g, ink.g, k); cb = mixf(bg.b, ink.b, k);
    }
    if (ringCov > 0) {
      cr_ = mixf(cr_, bg.r, ringCov); cg = mixf(cg, bg.g, ringCov); cb = mixf(cb, bg.b, ringCov);
      const float dc = clampf(bScreenR + .5f - dd, 0, 1);
      cr_ = mixf(cr_, 29, dc); cg = mixf(cg, 155, dc); cb = mixf(cb, 240, dc);
    }
    return pack(cr_, cg, cb);
  };

  // Two-level tiles: every field is 1-Lipschitz in screen pixels, so one sample at a tile centre
  // classifies it. 8x8 tiles that straddle an edge split into 4x4 quarters before shading.
  auto fillTile = [&](int x0, int y0, int x1, int y1, uint16_t c) {
    for (int y = y0; y < y1; ++y) { uint16_t *row = fb + y * W; for (int x = x0; x < x1; ++x) row[x] = c; }
  };
  auto doTile = [&](int x0, int y0, int x1, int y1, float reachT, bool leaf) -> bool {  // true = done
    const float mcx = (x0 + x1) * .5f, mcy = (y0 + y1) * .5f;
    float u, v;
    toLocal(mcx, mcy, u, v);
    float pd[4];
    const float db = (gm > .001f ? glyphParts(u, v, pd) : bodySdf(u, v)) * dpx;
    const int bodyMode = db > reachT ? 0 : (db < -reachT ? 2 : 1);
    const float bdx = mcx - bsx, bdy = mcy - bsy;
    const bool doBadge = showBadge && fsqrt(bdx * bdx + bdy * bdy) - bRingR < reachT;
    if (bodyMode == 0 && !doBadge) return true;
    int eyeMode = 0, eyeMask = 0;
    if (bodyMode && mcx > eyeX0 - 8 && mcx < eyeX1 + 8 && mcy > eyeY0 - 8 && mcy < eyeY1 + 8) {
      const float d0 = eyeOne(0, u, v) * dpx, d1 = eyeOne(1, u, v) * dpx;
      eyeMask = (d0 <= reachT ? 1 : 0) | (d1 <= reachT ? 2 : 0);
      const float de = fminf(d0, d1);
      eyeMode = de > reachT ? 0 : (de < -reachT ? 2 : 1);
    }
    if (gm > .001f) {  // glyph: flat only if every part is wholly in or out of this tile
      bool flat = !doBadge && eyeMode != 1;
      float keep = 1;
      for (int i = 0; i < 4 && flat; ++i) {
        if (pd[i] > 1e8f) continue;
        const float di = pd[i] * dpx;
        if (fabsf(di) <= reachT) flat = false;
        else if (di < 0) keep *= 1 - partA[i] * (i == 0 && eyeMode == 2 ? 0 : 1);
      }
      if (flat) {
        const float k = 1 - keep;
        if (k > 0) fillTile(x0, y0, x1, y1, pack(mixf(bg.r, ink.r, k), mixf(bg.g, ink.g, k), mixf(bg.b, ink.b, k)));
        return true;
      }
    } else if (!doBadge && bodyMode == 2 && eyeMode != 1) {
      fillTile(x0, y0, x1, y1, eyeMode == 2 ? bg565 : ink565);
      return true;
    }
    if (!leaf) return false;
    const int mask = eyeMode ? eyeMask : 0;
    for (int y = y0; y < y1; ++y) {
      uint16_t *row = fb + y * W;
      for (int x = x0; x < x1; ++x) {
        ++profShaded;
        const int c = shade(x, y, bodyMode, mask, doBadge);
        if (c >= 0) row[x] = uint16_t(c);
      }
    }
    return true;
  };
  constexpr float R8 = 4 * 1.4143f + 1.0f, R4 = 2 * 1.4143f + 1.0f;
  for (int ty = r.y0; ty < r.y1; ty += 8) {
    const int ty1 = min(ty + 8, r.y1);
    for (int tx = r.x0; tx < r.x1; tx += 8) {
      const int tx1 = min(tx + 8, r.x1);
      if (doTile(tx, ty, tx1, ty1, R8, false)) continue;
      for (int sy = ty; sy < ty1; sy += 4)
        for (int sx = tx; sx < tx1; sx += 4) doTile(sx, sy, min(sx + 4, tx1), min(sy + 4, ty1), R4, true);
    }
  }

  profTiles += micros() - pt0; pt0 = micros();
  if (ribbonCount) drawRibbons(true);
  for (int i = 0; i < partCount; ++i) {  // confetti
    const Particle &p = parts[i];
    const float a = clampf(1.8f - p.life, 0, 1);
    const RGB c = rgb(p.color);
    const float w = p.size * (0.4f + 0.6f * fabsf(cosf(p.rot)));
    canvas.fillSmoothRoundRect(int(p.x - w / 2), int(p.y - p.size / 2), max(2, int(w)), int(p.size), 2,
                               unswap(pack(mixf(bg.r, c.r, a), mixf(bg.g, c.g, a), mixf(bg.b, c.b, a))));
  }
  bandOn = focus >= 0;
  if (bandOn) drawBand(now, bg);
  if (labelOn) {
    const float a = clampf((labelUntil - now) / 400.0f, 0, 1);
    const RGB l = rgb(kLabel);
    canvas.setTextColor(unswap(pack(mixf(bg.r, l.r, a), mixf(bg.g, l.g, a), mixf(bg.b, l.b, a))));
    canvas.setFont(&fonts::FreeSansBold12pt7b);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextSize(1);
    canvas.drawString(labelText, W / 2, 400);
  }
  if (state == SSleeping) {
    canvas.setFont(&fonts::FreeSansBold12pt7b);
    canvas.setTextDatum(textdatum_t::middle_center);
    const float t = now / 1000.0f;
    for (int i = 0; i < 3; ++i) {
      const float ph = fmodf(t * .45f + i / 3.0f, 1), a = sinf(ph * PI) * .8f;
      canvas.setTextColor(unswap(pack(mixf(bg.r, 200, a), mixf(bg.g, 200, a), mixf(bg.b, 210, a))));
      canvas.setTextSize(.7f + ph * .6f);
      canvas.drawString("z", int(hcx + 80 + ph * 50), int(hcy - 110 - ph * 60));
    }
    canvas.setTextSize(1);
  }
  profExtra += micros() - pt0;
  prevRect = r;
  fullRedraw = false;
  return dirty;
}

// ---------- input ----------
static void wake() {
  manualSleep = false;
  if (focus < 0 && state == SSleeping) { demoState = SIdle; enterState(SIdle, false); }
}

static constexpr int kDemoCycle[] = {SIdle, SHappy, SCurious, SExcited, SSurprised, SThinking, SWorking,
                                     SWaiting, SBlocked, SDone, SSad};
static constexpr int kDemoCycleLen = sizeof(kDemoCycle) / sizeof(kDemoCycle[0]);

static void handleInput(c25k::Input in) {
  if (in != c25k::Input::None) lastInteractionMs = millis();
  if (focus >= 0) {  // pet mode: page through pi sessions
    switch (in) {
      case c25k::Input::Prev: wake(); focusPet(stepPet(focus, -1), true); break;  // left
      case c25k::Input::Next: wake(); focusPet(stepPet(focus, 1), true); break;   // right
      case c25k::Input::Enter:
        wake();
        pets[focus].unseen = false;
        startBounce();
        netOut.printf("GB_ACK %s\n", pets[focus].id);
        break;
      case c25k::Input::Escape: manualSleep = !manualSleep; break;
      default: break;
    }
    return;
  }
  switch (in) {  // demo mode
    case c25k::Input::Prev: wake(); demoStyle = (demoStyle + kStyleCount - 1) % kStyleCount; applyStyle(demoStyle, true); break;
    case c25k::Input::Next: wake(); demoStyle = (demoStyle + 1) % kStyleCount; applyStyle(demoStyle, true); break;
    case c25k::Input::Enter: {
      int k = 0;
      while (k < kDemoCycleLen && kDemoCycle[k] != state) ++k;
      demoState = state == SSleeping || k >= kDemoCycleLen ? SIdle : kDemoCycle[(k + 1) % kDemoCycleLen];
      enterState(demoState, true);
      // Audition each state's transition sound.
      if (demoState == SDone) playSound(SndChime);
      else if (demoState == SBlocked) playSound(SndUhoh);
      else if (demoState == SSurprised) playSound(SndWince);
      else if (demoState == SThinking || demoState == SWorking) playSound(SndStart);
      break;
    }
    case c25k::Input::Escape: demoState = state == SSleeping ? SIdle : SSleeping; enterState(demoState, true); break;
    default: break;
  }
}

static void handleButtons(uint32_t now) {
  handleInput(buttons.update(now, M5.BtnA.isPressed(), M5.BtnB.isPressed(),
                             M5.BtnA.wasClicked(), M5.BtnB.wasClicked()));
}

static bool onBody(float x, float y) {
  const float u = (x - (kHomeX + dragX.x)) / kScale, v = (y - (kHomeY + dragY.x)) / kScale;
  return sampleSdf(sdfCur, u + C, v + C) < 14;
}

static void handleTouch(uint32_t now) {
  lgfx::touch_point_t raw;
  const bool pressed = M5.Display.getTouchRaw(&raw) > 0;
  if (pressed) {
    auto p = touchcal::applyWarp(touchMap, {double(raw.x), double(raw.y)});
    touchX = clampf(p.x, 0, W - 1); touchY = clampf(p.y, 0, H - 1);
    lastTouchMs = now;
    lastInteractionMs = now;
    if (!touchDown) {
      touchDown = true;
      touchStartMs = now; touchStartX = touchX; touchStartY = touchY;
      wake();
      dragging = false;
      grabDX = touchX - (kHomeX + dragX.x); grabDY = touchY - (kHomeY + dragY.x);
    } else if (!dragging && onBody(touchStartX, touchStartY) && hypotf(touchX - touchStartX, touchY - touchStartY) > 14) {
      dragging = true;
      netOut.println("GB_EVENT drag");
    }
  } else if (touchDown) {
    touchDown = false;
    const bool tap = now - touchStartMs < 350 && hypotf(touchX - touchStartX, touchY - touchStartY) < 14;
    if (dragging) { sqy.v -= 4; sqx.v += 3.5f; dragging = false; }
    else if (tap && onBody(touchX, touchY)) { startBounce(); if (frand(0, 1) < .3f) startSpin(); netOut.println("GB_EVENT bounce"); }
    else if (tap) {
      const float dx = touchX - kHomeX, dy = touchY - kHomeY, len = fmaxf(hypotf(dx, dy), 1);
      gazeX.t = dx / len * 15; gazeY.t = dy / len * 9;
      gazeHoldUntil = now + 1800;
      netOut.printf("GB_EVENT look x=%d y=%d\n", int(touchX), int(touchY));
    }
  }
}

// Tilt: the pet sloshes when the board's orientation changes, then drifts back to centre, so any
// resting pose (flat, upright on its lanyard, leaning) keeps it centred.
// Axis mapping measured on the device, lanyard down and standing upright: ax = -0.83, so IMU +x
// points toward the lanyard (screen down) and, right-handed with +z out of the screen, IMU +y
// points screen-left. "Downhill" in screen space is (ay, -ax).
static void handleImu(uint32_t now) {
  if (!imuOk || !M5.Imu.update()) return;
  static uint32_t lastMs = 0;
  static float baseX = 0, baseY = 0;
  static bool haveBase = false;
  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);
  const float downX = ay, downY = -ax;  // downhill direction on the screen (in g)
  const float dt = lastMs ? clampf((now - lastMs) / 1000.0f, 0, .2f) : 0;
  lastMs = now;
  if (!haveBase) { baseX = downX; baseY = downY; haveBase = true; }
  const float k = 1 - expf(-dt / 2.5f);  // baseline follows the resting pose (tau 2.5 s)
  baseX += (downX - baseX) * k; baseY += (downY - baseY) * k;
  float tx = downX - baseX, ty = downY - baseY;
  if (fabsf(tx) < .05f) tx = 0;          // deadband: sensor noise never nudges the pet
  if (fabsf(ty) < .05f) ty = 0;
  tiltX = mixf(tiltX, clampf(tx * 1.5f, -1, 1), .2f);
  tiltY = mixf(tiltY, clampf(ty * 1.5f, -1, 1), .2f);
  const float mag = sqrtf(ax * ax + ay * ay + az * az);
  if (mag > 2.2f && state != SSleeping) { startBounce(); startSpin(); }
  if (now - lastImuLog > 5000) {
    lastImuLog = now;
    netOut.printf("GB_IMU ax=%.2f ay=%.2f az=%.2f tilt=%.2f,%.2f\n", ax, ay, az, tiltX, tiltY);
  }
}

// Frame recorder (evidence): N frames of a cropped, downsampled region at the loop's own rate.
static uint16_t *recBuf = nullptr;
static int recN = 0, recI = 0, recX = 0, recY = 0, recSize = 0, recStep = 2, recOut = 0;
static uint32_t *recT = nullptr;
static void recordFrame(uint32_t now) {
  if (!recBuf || recI >= recN) return;
  uint16_t *dst = recBuf + size_t(recI) * recOut * recOut;
  for (int y = 0; y < recOut; ++y) {
    const uint16_t *row = fb + (recY + y * recStep) * W + recX;
    for (int x = 0; x < recOut; ++x) dst[y * recOut + x] = row[x * recStep];
  }
  recT[recI++] = now;
  if (recI == recN) Serial.printf("GB_REC_DONE n=%d\n", recN);
}
static void dumpRec() {
  Serial.setTxTimeoutMs(2000);  // a reader asked for this dump: let bulk writes wait for it
  Serial.printf("GB_REC %d %d\n", recI, recOut);
  for (int i = 0; i < recI; ++i) Serial.printf("%u%c", recT[i], i + 1 < recI ? ' ' : '\n');
  Serial.flush();
  const uint8_t *p = reinterpret_cast<const uint8_t *>(recBuf);
  const size_t total = size_t(recI) * recOut * recOut * 2;
  for (size_t off = 0; off < total; off += 4096) Serial.write(p + off, min<size_t>(4096, total - off));
  Serial.flush();
  Serial.println("\nGB_REC_END");
  Serial.setTxTimeoutMs(0);
}

static void dumpFrame() {
  Serial.setTxTimeoutMs(2000);  // a reader asked for this dump: let bulk writes wait for it
  Serial.printf("GB_SHOT %d %d\n", W, H);
  Serial.flush();
  const uint8_t *p = reinterpret_cast<const uint8_t *>(fb);
  const size_t total = size_t(W) * H * 2;
  for (size_t off = 0; off < total; off += 4096) Serial.write(p + off, min<size_t>(4096, total - off));
  Serial.flush();
  Serial.println("\nGB_SHOT_END");
  Serial.setTxTimeoutMs(0);
}

static int stateByName(const char *name) {
  for (int i = 0; i < kStateCount; ++i) if (!strcmp(kStateNames[i], name)) return i;
  return -1;
}

static void handleSerial() {
  static char line[200];
  static size_t n = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n' && c != '\r') { if (n < sizeof(line) - 1) line[n++] = c; continue; }
    line[n] = 0;
    n = 0;
    processLine(line, true);
  }
}

// One protocol line from USB (fromUsb) or from the authenticated Wi-Fi link.
static void processLine(char *line, bool fromUsb) {
  {
    lastHubMs = millis();
    if (!strcmp(line, "ping")) return;  // hub heartbeat
    if (!strcmp(line, "hello")) { netlink::hello(netOut); return; }
    if (!strncmp(line, "wifi ", 5) || !strncmp(line, "pair ", 5) || !strncmp(line, "hubaddr ", 8) || !strcmp(line, "forget")) {
      if (fromUsb) netlink::provision(line, netOut);  // provisioning only over the trusted USB link
      else netOut.println("GB_ERROR usb_only");
      return;
    }
    if (!strcmp(line, "hop") || !strcmp(line, "bounce")) startBounce();
    else if (!strcmp(line, "spin")) startSpin();
    else if (!strcmp(line, "burst")) burst(22);
    else if (!strncmp(line, "sound ", 6)) {
      for (int k = 0; k < kSoundCount; ++k) if (!strcmp(line + 6, kSoundNames[k])) playSound(k);
    }
    else if (!strcmp(line, "blink")) blinkNow(millis());
    else if (!strcmp(line, "shot")) dumpFrame();
    else if (!strncmp(line, "rec ", 4)) {  // rec <frames> <x0> <y0> <size> <step>
      int n2, x0, y0, sz, st;
      if (sscanf(line + 4, "%d %d %d %d %d", &n2, &x0, &y0, &sz, &st) == 5 && st > 0) {
        free(recBuf); free(recT);
        recOut = sz / st; recN = n2; recI = 0; recX = x0; recY = y0; recSize = sz; recStep = st;
        recBuf = static_cast<uint16_t *>(ps_malloc(size_t(n2) * recOut * recOut * 2));
        recT = static_cast<uint32_t *>(ps_malloc(n2 * 4));
        Serial.printf("GB_REC_START ok=%d\n", recBuf != nullptr);
      }
    } else if (!strcmp(line, "recdump")) dumpRec();
    else if (!strcmp(line, "next")) handleInput(c25k::Input::Next);
    else if (!strcmp(line, "prev")) handleInput(c25k::Input::Prev);
    else if (!strncmp(line, "expr ", 5)) { setExpr(atoi(line + 5) % kExprCount, 7); nextExpr = millis() + 60000; }
    else if (!strncmp(line, "style ", 6)) { demoStyle = atoi(line + 6) % kStyleCount; applyStyle(demoStyle, true); }
    else if (!strncmp(line, "mood ", 5)) {
      const int s = stateByName(line + 5);
      if (s < 0) netOut.printf("GB_ERROR unknown_mood=%s\n", line + 5);
      else if (focus < 0) { demoState = s; enterState(s, true); }
    } else if (!strncmp(line, "look ", 5)) {
      float x, y;
      if (sscanf(line + 5, "%f %f", &x, &y) == 2) { gazeX.t = clampf(x, -1, 1) * 15; gazeY.t = clampf(y, -1, 1) * 9; gazeHoldUntil = millis() + 5000; }
    } else if (!strncmp(line, "pet ", 4)) handlePetLine(line + 4);
    else if (!strncmp(line, "unpet ", 6)) removePet(line + 6);
    else if (!strcmp(line, "pets")) {
      netOut.printf("GB_PETS n=%d focus=%s mood=%s manual_sleep=%d demo_bot=%d",
                    petCount(), focus >= 0 ? pets[focus].id : "-", kStateNames[state], manualSleep, demoStyle);
      for (auto &p : pets) if (p.used) netOut.printf(" %s:%s:%d", p.id, kPetStateNames[int(p.state)], p.unseen);
      netOut.println();
    } else if (!strncmp(line, "act ", 4)) {
      const char *a = line + 4;
      handleInput(!strcmp(a, "left") ? c25k::Input::Prev : !strcmp(a, "right") ? c25k::Input::Next
                  : !strcmp(a, "enter") ? c25k::Input::Enter : !strcmp(a, "hold") ? c25k::Input::Escape
                  : c25k::Input::None);
    } else if (!strcmp(line, "reset")) {
      demoState = SIdle; demoStyle = 0;
      removePet("*");
      manualSleep = false;
      applyStyle(0, false);
      enterState(SIdle, false);
    }
  }
}

// ---------- setup / loop ----------
void setup() {
  Serial.begin(115200);
  // Never block on USB: with the host port closed (hub released it, or a charger-only
  // connection) CDC writes would otherwise wait out a timeout per call and stall the loop.
  Serial.setTxTimeoutMs(0);
  auto cfg = M5.config();
  cfg.internal_imu = true;
  cfg.internal_rtc = false;
  cfg.internal_mic = false;
  cfg.internal_spk = true;
  cfg.fallback_board = m5::board_t::board_M5StopWatch;
  M5.begin(cfg);
  auto *panel = M5.Display.getPanel();
  auto pc = panel->config();
  pc.panel_width = 468; pc.panel_height = 466; pc.offset_x = 6; pc.offset_y = 0;
  panel->config(pc);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(255);
  netOut.printf("GB_HARDWARE board=%d expected=%d size=%dx%d touch=%d psram=%u\n",
                int(M5.getBoard()), int(m5::board_t::board_M5StopWatch),
                M5.Display.width(), M5.Display.height(), M5.Touch.isEnabled(), ESP.getPsramSize());
  const bool mapOk = loadTouchMap();
  netOut.printf("GB_TOUCHMAP ok=%d version=%u generation=%u\n", mapOk, touchMapVersion, touchMapGeneration);
  imuOk = M5.Imu.isEnabled();
  const bool spk = M5.Speaker.begin();
  M5.Speaker.setVolume(170);
  initSounds();
  netOut.printf("GB_SPEAKER ok=%d\n", spk);
  netlink::begin();
  netlink::hello(netOut);

  canvas.setPsram(true);
  canvas.setColorDepth(16);
  fb = static_cast<uint16_t *>(canvas.createSprite(W, H));
  if (!fb) { netOut.println("GB_FATAL canvas"); while (true) delay(1000); }

  d.init(0); o.init(0); A.init(0); h.init(1);
  lid.init(1); eyeScale.init(1); gazeX.init(0); gazeY.init(0);
  blend.init(1); glyphMix.init(0); headMix.init(1); spin.init(0); styleMix.init(0); eyeTurn.init(0);
  sqx.init(1); sqy.init(1); dragX.init(0); dragY.init(0);
  const RGB c = rgb(kStyles[0].ink);
  inkR.init(c.r); inkG.init(c.g); inkB.init(c.b);
  memcpy(sdfCur, kHeadSdf[0], sizeof(sdfCur)); memcpy(sdfPrev, sdfCur, sizeof(sdfCur));
  exprTo[0] = exprFrom[0] = exprCap(0, 0); exprTo[1] = exprFrom[1] = exprCap(0, 1);
  state = -1;
  enterState(SIdle, false);
  sqx.x = 1.3f; sqy.x = .7f;  // entrance pop
  startBounce();
  showLabel(kStyles[0].name);
  netOut.println("GB_READY");
}

void loop() {
  static uint32_t last = millis(), fpsStart = millis(), frames = 0, renderUs = 0, busyUs = 0;
  M5.update();
  const uint32_t now = millis();
  const float dt = clampf((now - last) / 1000.0f, .001f, .05f);
  last = now;

  handleButtons(now);
  handleTouch(now);
  handleImu(now);
  handleSerial();
  netlink::poll();
  tick(dt, now);

  const uint32_t t0 = micros();
  const Rect r = render(now);
  renderUs += micros() - t0;
  M5.Display.setClipRect(r.x0, r.y0, r.x1 - r.x0, r.y1 - r.y0);
  canvas.pushSprite(0, 0);
  if (bandOn && !(r.x0 == 0 && r.y0 == 0 && r.x1 == W && r.y1 == H)) {
    M5.Display.setClipRect(kBand.x0, kBand.y0, kBand.x1 - kBand.x0, kBand.y1 - kBand.y0);
    canvas.pushSprite(0, 0);
  }
  M5.Display.clearClipRect();
  busyUs += micros() - t0;
  recordFrame(now);

  if (++frames, now - fpsStart >= 5000) {
    netOut.printf("GB_FPS fps=%.1f frame_ms=%.1f render_ms=%.1f fill=%.1f tiles=%.1f extra=%.1f shaded=%u poly=%u heap=%u stack_free=%u\n",
                  frames * 1000.0f / (now - fpsStart), busyUs / 1000.0f / frames, renderUs / 1000.0f / frames,
                  profFill / 1000.0f / frames, profTiles / 1000.0f / frames, profExtra / 1000.0f / frames, profShaded / frames, profPoly / frames,
                  ESP.getFreeHeap(), uxTaskGetStackHighWaterMark(nullptr));
    frames = 0; renderUs = 0; busyUs = 0; fpsStart = now; profFill = profTiles = profExtra = profShaded = profPoly = 0;
  }
}
