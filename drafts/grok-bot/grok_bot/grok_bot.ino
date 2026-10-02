// Grok Bot companion for the M5Stack Stopwatch (disposable toy build, v2).
// A soft blob with two expressive eyes: breathes, blinks, darts its gaze,
// hops, emotes, and follows touch / tilt. Visual language from x.ai's
// "Designing Grok Bot" guide.
//
// Controls (lanyard at the bottom): left (BtnA) = previous bot,
// right (BtnB) = next bot, both tapped = next mood, both held 600 ms = sleep/wake.
// Touch: tap the bot to hop, drag it around, tap elsewhere to make it look.
#pragma GCC optimize("O2")
#include <M5Unified.h>
#include <Preferences.h>
#include <math.h>
#include "buttons.h"
#include "types.h"
#include "touch_calibration_core.h"

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

// ---------- small math ----------
static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline float mixf(float a, float b, float t) { return a + (b - a) * t; }
static inline float frand(float a, float b) { return a + (b - a) * (esp_random() / 4294967295.0f); }

struct Spring {
  float x = 0, v = 0, target = 0, k = 200, c = 20;
  void init(float value, float stiff, float damp) { x = target = value; v = 0; k = stiff; c = damp; }
  void step(float dt) {
    while (dt > 0) {  // semi-implicit Euler in 4 ms substeps
      float h = dt > 0.004f ? 0.004f : dt;
      v += (k * (target - x) - c * v) * h;
      x += v * h;
      dt -= h;
    }
  }
};

static RGB rgb(uint32_t hex) { return {float((hex >> 16) & 255), float((hex >> 8) & 255), float(hex & 255)}; }

// ---------- characters ----------
struct Bot {
  const char *name;
  float hw, hh, rad;  // rounded-box half width/height and corner radius
  uint32_t body, eye, bg, label;
};
static const Bot kBots[] = {
  {"Grok Bot",    100, 100, 100, 0x6E5BE8, 0x16140F, 0x14130F, 0xB9B4A8},
  {"Experiments", 132,  86,  86, 0xF43B50, 0xFFFFFF, 0x14130F, 0xB9B4A8},
  {"Motion God",  106, 100, 100, 0x2F8FF6, 0xFFFFFF, 0x14130F, 0xB9B4A8},
  {"DevBot",      100,  96,  44, 0x0B0B0B, 0xFFFFFF, 0xF3F3F1, 0x6B6B66},
};
static constexpr int kBotCount = sizeof(kBots) / sizeof(kBots[0]);
static int botIndex = 0;

// ---------- moods ----------
// Eye geometry is in units of the body's half height. `arc` morphs the pill
// into a ^ arc; `tilt` slants the eyes symmetrically (+ = "/ \" worried).
enum class Gaze : uint8_t { Wander, Up, Read, Center, Down };
struct Mood {
  const char *name;
  float len, rad, arc, tilt, open, spread, spin;  // spin rotates both eyes (sleepy = flat)
  Gaze gaze;
  float breathAmp, breathPeriod;
  uint32_t hopEveryMs;  // 0 = never on its own
};
enum MoodId { Neutral, Happy, Surprised, Thinking, Busy, Worried, Attention, Sleepy, kMoodCount };
static const Mood kMoods[kMoodCount] = {
  //  name        len   rad   arc  tilt  open  spread spin  gaze          amp    period hop
  {"neutral",   0.15f, 0.09f, 0, 0.00f, 1.00f, 1.00f, 0.00f, Gaze::Wander, 0.022f, 2.8f, 15000},
  {"happy",     0.00f, 0.07f, 1, 0.00f, 1.00f, 1.05f, 0.00f, Gaze::Center, 0.035f, 1.4f, 3200},
  {"surprised", 0.10f, 0.13f, 0, 0.00f, 1.15f, 1.12f, 0.00f, Gaze::Center, 0.015f, 2.0f, 0},
  {"thinking",  0.05f, 0.08f, 0, 0.00f, 1.00f, 0.95f, 0.00f, Gaze::Up,     0.030f, 3.6f, 0},
  {"busy",      0.12f, 0.09f, 0, 0.00f, 1.00f, 0.95f, 0.00f, Gaze::Read,   0.012f, 1.2f, 0},
  {"worried",   0.13f, 0.08f, 0, 0.38f, 1.00f, 1.00f, 0.00f, Gaze::Center, 0.018f, 1.6f, 0},
  {"attention", 0.17f, 0.10f, 0, 0.00f, 1.08f, 1.05f, 0.00f, Gaze::Center, 0.020f, 2.0f, 2400},
  {"sleepy",    0.07f, 0.045f,0, 0.00f, 1.00f, 1.00f, 1.50f, Gaze::Down,   0.045f, 4.2f, 0},
};
static int moodIndex = Neutral;

// ---------- animation state ----------
static Spring hw, hh, rad;                 // shape morph (jelly)
static Spring bodyR, bodyG, bodyB, eyeR, eyeG, eyeB, bgR, bgG, bgB;
static Spring sx, sy;                      // squash / stretch
static Spring ox, oy;                      // body offset from home (lags the eyes)
static Spring gx, gy;                      // gaze, unit disc (snappy, slight overshoot)
static Spring eyeOpen;                     // blink squash, overshoots when reopening
static Spring eLen, eRad, eArc, eTilt, eSpread, eSpin;
static float hop = 0, hopV = 0, hopPower = 820;
static bool airborne = false;
static uint32_t anticipateUntil = 0;
static uint32_t nextBlink = 0, blinkUntil = 0, secondBlinkAt = 0;
static uint32_t nextGlance = 0, nextMicro = 0, nextMoodHop = 0, nextIdleHop = 0;
static float holdGx = 0, holdGy = 0;       // current fixation (micro-saccades jitter around it)
static int readStep = 0;
static uint32_t labelUntil = 0;
static char labelText[32] = "";
static bool sleeping = false;

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
static constexpr float kHomeX = 234, kHomeY = 240;

static void showLabel(const char *text) {
  strncpy(labelText, text, sizeof(labelText) - 1);
  labelUntil = millis() + 1600;
}

static void applyBot(int index, bool pop) {
  botIndex = (index + kBotCount) % kBotCount;
  const Bot &b = kBots[botIndex];
  hw.target = b.hw; hh.target = b.hh; rad.target = b.rad;
  RGB c = rgb(b.body), e = rgb(b.eye), g = rgb(b.bg);
  bodyR.target = c.r; bodyG.target = c.g; bodyB.target = c.b;
  eyeR.target = e.r; eyeG.target = e.g; eyeB.target = e.b;
  bgR.target = g.r; bgG.target = g.g; bgB.target = g.b;
  if (pop) { sy.v -= 5.0f; sx.v += 4.0f; }
  showLabel(b.name);
  Serial.printf("GB_EVENT bot=%s\n", b.name);
}

static void startHop(float power = 820) {
  if (airborne || anticipateUntil || sleeping) return;
  hopPower = power;
  anticipateUntil = millis() + 130;
}

// Move the fixation point: eyes snap there, the body gets a small nudge.
static void saccade(float x, float y) {
  const float dx = x - gx.target;
  holdGx = gx.target = clampf(x, -1, 1);
  holdGy = gy.target = clampf(y, -1, 1);
  ox.v += dx * 70.0f;
  sx.v += fabsf(dx) * 1.2f;
}

static void applyMood(int index, bool announce = true) {
  moodIndex = (index + kMoodCount) % kMoodCount;
  const Mood &m = kMoods[moodIndex];
  eLen.target = m.len; eRad.target = m.rad; eArc.target = m.arc;
  eTilt.target = m.tilt; eSpread.target = m.spread; eSpin.target = m.spin;
  const bool wasSleeping = sleeping;
  sleeping = moodIndex == Sleepy;
  if (sleeping != wasSleeping) M5.Display.setBrightness(sleeping ? 70 : 255);
  const uint32_t now = millis();
  nextGlance = now;
  nextMoodHop = m.hopEveryMs ? now + 300 : 0;
  readStep = 0;
  switch (moodIndex) {  // entrance beat for each mood
    case Surprised: sy.v += 7.0f; sx.v -= 5.0f; eyeOpen.v += 6.0f; break;
    case Happy: startHop(700); break;
    case Worried: sx.v -= 3.0f; break;
    case Attention: startHop(900); break;
    default: sy.v -= 2.5f; sx.v += 2.0f; break;
  }
  if (announce) showLabel(m.name);
  Serial.printf("GB_EVENT mood=%s\n", m.name);
}

static void blink(uint32_t now) {
  if (sleeping) return;
  blinkUntil = now + 90;
  if (frand(0, 1) < 0.2f) secondBlinkAt = now + 240;
  nextBlink = now + uint32_t(frand(2000, 5200));
}

// ---------- physics tick ----------
static void updateGaze(uint32_t now) {
  const Mood &m = kMoods[moodIndex];
  if (touchDown) {
    float dx = touchX - (kHomeX + ox.x), dy = touchY - (kHomeY + oy.x - hop);
    float len = sqrtf(dx * dx + dy * dy);
    float s = clampf(len / 140.0f, 0, 1) / (len > 1 ? len : 1);
    gx.target = dragging ? clampf(grabDX * -0.004f, -0.4f, 0.4f) : dx * s;
    gy.target = dragging ? 0.1f : dy * s;
    return;
  }
  if (m.gaze == Gaze::Wander && (fabsf(tiltX) > 0.18f || fabsf(tiltY) > 0.18f)) {
    gx.target = clampf(tiltX * 1.6f, -1, 1);
    gy.target = clampf(tiltY * 1.6f, -1, 1);
    nextGlance = now + 600;
    return;
  }
  if (now >= nextGlance) {
    switch (m.gaze) {
      case Gaze::Wander:
        if (frand(0, 1) < 0.3f) saccade(0, 0);
        else { float a = frand(0, 2 * PI), r = frand(0.5f, 1.0f); saccade(cosf(a) * r, sinf(a) * r * 0.8f); }
        nextGlance = now + uint32_t(frand(700, 2600));
        break;
      case Gaze::Up:  // pondering: up and off to one side, switching sides now and then
        saccade(frand(0, 1) < 0.5f ? frand(0.45f, 0.8f) : -frand(0.45f, 0.8f), -frand(0.6f, 0.9f));
        nextGlance = now + uint32_t(frand(1400, 3000));
        break;
      case Gaze::Read: {  // reading: hop across a line in steps, then return to the next line
        const int col = readStep % 5, line = (readStep / 5) % 3;
        saccade(-0.7f + col * 0.35f, -0.25f + line * 0.25f);
        nextGlance = now + (col == 4 ? 260 : uint32_t(frand(140, 230)));
        ++readStep;
        break;
      }
      case Gaze::Center:
        saccade(frand(-0.08f, 0.08f), -0.05f);
        nextGlance = now + uint32_t(frand(1500, 3500));
        break;
      case Gaze::Down:
        saccade(0, 0.35f);
        nextGlance = now + 5000;
        break;
    }
    nextMicro = now + uint32_t(frand(250, 600));
  } else if (now >= nextMicro && m.gaze != Gaze::Read && m.gaze != Gaze::Down) {
    // Micro-saccades: tiny darts around the fixation keep a still stare alive.
    gx.target = holdGx + frand(-0.06f, 0.06f);
    gy.target = holdGy + frand(-0.05f, 0.05f);
    nextMicro = now + uint32_t(frand(300, 800));
  }
}

static void tick(float dt, uint32_t now) {
  const float t = now / 1000.0f;
  const Mood &m = kMoods[moodIndex];

  if (!sleeping && now >= nextBlink) blink(now);
  if (secondBlinkAt && now >= secondBlinkAt) { secondBlinkAt = 0; blinkUntil = now + 80; }
  eyeOpen.target = now < blinkUntil ? 0.08f : m.open;

  updateGaze(now);

  // Body follows the eyes a beat later, slides downhill, or follows a drag.
  if (dragging) {
    ox.target = clampf(touchX - grabDX - kHomeX, -150, 150);
    oy.target = clampf(touchY - grabDY - kHomeY, -140, 110);
  } else {
    ox.target = gx.x * 14.0f + clampf(tiltX, -1, 1) * 70.0f;
    oy.target = gy.x * 8.0f + clampf(tiltY, -1, 1) * 55.0f + (sleeping ? 14.0f : 0.0f);
  }

  const float breath = sinf(t * 2 * PI / m.breathPeriod);
  float sxT = 1.0f - breath * m.breathAmp * 0.7f;
  float syT = 1.0f + breath * m.breathAmp;
  if (moodIndex == Thinking) sxT += 0.02f * sinf(t * 1.3f);   // slow ponder wobble
  if (moodIndex == Worried) sxT += 0.012f * sinf(t * 31.0f);  // nervous shiver

  // Hop: anticipation squash -> ballistic flight with stretch -> landing squash.
  if (anticipateUntil) {
    sxT = 1.14f; syT = 0.84f;
    if (now >= anticipateUntil) {
      anticipateUntil = 0;
      airborne = true;
      hopV = hopPower;
      sy.v += 6.0f; sx.v -= 4.0f;
      eyeOpen.v += 4.0f;
    }
  }
  if (airborne) {
    hopV -= 3300 * dt;
    hop += hopV * dt;
    float s = clampf(fabsf(hopV) / hopPower, 0, 1);
    sxT = 1.0f - 0.10f * s; syT = 1.0f + 0.13f * s;
    if (hop <= 0) {
      hop = 0; airborne = false;
      sy.v -= fabsf(hopV) * 0.009f; sx.v += fabsf(hopV) * 0.007f;
      eyeOpen.v -= fabsf(hopV) * 0.006f;  // eyes squish on impact
      hopV = 0;
    }
  }
  if (dragging) {
    float d = clampf(sqrtf(ox.v * ox.v + oy.v * oy.v) / 1500.0f, 0, 0.12f);
    sxT += d; syT -= d * 0.6f;
  }
  sx.target = sxT; sy.target = syT;

  if (!touchDown && nextMoodHop && now >= nextMoodHop) {
    startHop(moodIndex == Happy ? 620 : 760);
    nextMoodHop = now + m.hopEveryMs + uint32_t(frand(0, 800));
  }
  if (moodIndex == Neutral && !touchDown && now >= nextIdleHop) {
    if (now - lastTouchMs > 4000) startHop();
    nextIdleHop = now + uint32_t(frand(9000, 20000));
  }

  Spring *all[] = {&hw, &hh, &rad, &bodyR, &bodyG, &bodyB, &eyeR, &eyeG, &eyeB, &bgR, &bgG, &bgB,
                   &sx, &sy, &ox, &oy, &gx, &gy, &eyeOpen, &eLen, &eRad, &eArc, &eTilt, &eSpread, &eSpin};
  for (Spring *s : all) s->step(dt);
  sx.x = clampf(sx.x, 0.6f, 1.5f);
  sy.x = clampf(sy.x, 0.6f, 1.5f);
}

// ---------- rendering ----------
static inline uint16_t pack(float r, float g, float b) {
  uint16_t c = (uint16_t(clampf(r, 0, 255)) >> 3) << 11 | (uint16_t(clampf(g, 0, 255)) >> 2) << 5 |
               (uint16_t(clampf(b, 0, 255)) >> 3);
  return uint16_t((c >> 8) | (c << 8));  // sprite buffer is byte-swapped RGB565
}
static inline uint16_t unswap(uint16_t c) { return uint16_t((c >> 8) | (c << 8)); }

static inline float sdRoundBox(float px, float py, float bx, float by, float r) {
  float qx = fabsf(px) - bx + r, qy = fabsf(py) - by + r;
  float ax = qx > 0 ? qx : 0, ay = qy > 0 ? qy : 0;
  float m = qx > qy ? qx : qy;
  return sqrtf(ax * ax + ay * ay) + (m < 0 ? m : 0) - r;
}

// One eye in its own frame: `along` runs down the eye, `perp` across it.
static inline float sdEye(float perp, float along, const EyeShape &e) {
  float dCap = 0, dArc = 0;
  if (e.arc < 0.999f) {
    const float a = fabsf(along) - e.len;
    const float ya = a > 0 ? a : 0;
    dCap = sqrtf(perp * perp + ya * ya) - e.rad;
  }
  if (e.arc > 0.001f) {  // ^ arc (iq's sdArc with the opening facing down)
    const float u = fabsf(perp), v = -along + e.arcR * 0.45f;
    dArc = (e.cosA * u > e.sinA * v) ? sqrtf((u - e.sinA * e.arcR) * (u - e.sinA * e.arcR) +
                                             (v - e.cosA * e.arcR) * (v - e.cosA * e.arcR))
                                     : fabsf(sqrtf(u * u + v * v) - e.arcR);
    dArc -= e.arcW;
  }
  return e.arc <= 0.001f ? dCap : (e.arc >= 0.999f ? dArc : mixf(dCap, dArc, e.arc));
}

static Rect prevRect = {0, 0, W, H};
static bool fullRedraw = true;
static uint32_t lastBg = 0xFFFFFFFF;

static Rect render(uint32_t now) {
  const float cx = kHomeX + ox.x;
  const float bw = clampf(hw.x, 30, 180), bh = clampf(hh.x, 30, 180);
  const float br = clampf(rad.x, 4, fminf(bw, bh));
  const float feet = kHomeY + oy.x - hop + bh;  // squash anchored at the feet
  const float cy = feet - bh * sy.x;
  const float isx = 1.0f / sx.x, isy = 1.0f / sy.x;
  const float dscale = fminf(sx.x, sy.x);

  RGB body = {bodyR.x, bodyG.x, bodyB.x}, eye = {eyeR.x, eyeG.x, eyeB.x}, bg = {bgR.x, bgG.x, bgB.x};
  const uint16_t bg565 = pack(bg.r, bg.g, bg.b);
  const bool labelOn = now < labelUntil;
  const bool full = fullRedraw || bg565 != lastBg || labelOn || sleeping;
  lastBg = bg565;

  // ---- eyes (body-local coordinates; they squash with the body) ----
  const float open = clampf(eyeOpen.x, 0.08f, 1.35f);
  const float gazeX = gx.x, gazeY = gy.x;
  const float spread = 0.26f * bh * eSpread.x * (1.0f - 0.18f * fabsf(gazeX));  // turn = eyes close up
  const float ecx = gazeX * 0.32f * bw, ecy = 0.10f * bh + gazeY * 0.26f * bh;
  struct EyeInst { float x, y; EyeShape s; } eyes[2];
  for (int i = 0; i < 2; ++i) {
    const float side = i == 0 ? -1.0f : 1.0f;
    // Slight built-in asymmetry plus perspective: the far eye shrinks a bit.
    const float persp = 1.0f - 0.10f * gazeX * side;
    const float asym = i == 0 ? 1.0f : 0.94f;
    EyeShape &s = eyes[i].s;
    s.len = fmaxf(eLen.x, 0) * bh * persp * asym;
    s.rad = eRad.x * bh * persp;
    s.arc = clampf(eArc.x, 0, 1);
    s.arcR = 0.19f * bh * persp;
    s.arcW = 0.056f * bh;
    s.sinA = sinf(1.15f); s.cosA = cosf(1.15f);
    eyes[i].x = ecx + side * spread;
    eyes[i].y = ecy;
  }
  float eyeSin[2], eyeCos[2];
  for (int i = 0; i < 2; ++i) {
    const float side = i == 0 ? -1.0f : 1.0f;
    const float calm = (1.0f - clampf(eSpin.x / 1.5f, 0, 1)) * (1.0f - clampf(fabsf(eTilt.x) * 2.2f, 0, 0.85f));
    const float angle = (0.10f + gazeX * 0.30f) * calm + eSpin.x +
                        side * eTilt.x + (i == 0 ? 0.05f : -0.02f);
    eyeSin[i] = sinf(angle); eyeCos[i] = cosf(angle);
  }
  // Eye distance in screen pixels (1-Lipschitz: blink squash only shrinks it).
  auto eyeDist = [&](float lx, float ly) {
    const int i = lx < ecx ? 0 : 1;
    const float qx = lx - eyes[i].x, qy = (ly - eyes[i].y) / open;
    const float along = qx * eyeSin[i] + qy * eyeCos[i];
    const float perp = qx * eyeCos[i] - qy * eyeSin[i];
    return sdEye(perp, along, eyes[i].s) * fminf(open, 1.0f) * dscale;
  };
  const float eyeReach = 0.34f * bh;
  const float eyeX0 = cx + (ecx - spread - eyeReach) * sx.x, eyeX1 = cx + (ecx + spread + eyeReach) * sx.x;
  const float eyeY0 = cy + (ecy - eyeReach * 1.4f) * sy.x, eyeY1 = cy + (ecy + eyeReach * 1.4f) * sy.x;

  // Online dot, screen space, lower right of the body.
  const float dotR = 0.11f * bh, ringR = dotR + 4.5f;
  const float dotX = cx + 0.66f * bw * sx.x, dotY = cy + 0.66f * bh * sy.x;
  const float gr = sleeping ? 150 : 34, gg = sleeping ? 150 : 197, gb = sleeping ? 150 : 94;

  Rect r;
  r.x0 = int(floorf(cx - bw * sx.x)) - 3;
  r.x1 = int(ceilf(fmaxf(cx + bw * sx.x, dotX + ringR))) + 3;
  r.y0 = int(floorf(cy - bh * sy.x)) - 3;
  r.y1 = int(ceilf(fmaxf(cy + bh * sy.x, dotY + ringR))) + 3;
  r.x0 = max(r.x0, 0); r.y0 = max(r.y0, 0); r.x1 = min(r.x1, W); r.y1 = min(r.y1, H);

  Rect dirty = full ? Rect{0, 0, W, H}
                    : Rect{min(r.x0, prevRect.x0), min(r.y0, prevRect.y0),
                           max(r.x1, prevRect.x1), max(r.y1, prevRect.y1)};
  canvas.fillRect(dirty.x0, dirty.y0, dirty.x1 - dirty.x0, dirty.y1 - dirty.y0, unswap(bg565));

  const uint16_t body565 = pack(body.r, body.g, body.b);
  const uint16_t eye565 = pack(eye.r, eye.g, eye.b);

  // mode: 0 outside/none, 1 edge (per-pixel), 2 inside/solid
  auto shade = [&](int x, int y, int bodyMode, int eyeMode, bool dot) -> int {  // -1 = keep bg
    const float ly = (y + 0.5f - cy) * isy;
    const float lx = (x + 0.5f - cx) * isx;
    float cov = bodyMode == 2 ? 1.0f : 0.0f;
    if (bodyMode == 1) cov = clampf(0.5f - sdRoundBox(lx, ly, bw, bh, br) * dscale, 0, 1);
    float ringCov = 0, dd = 0;
    if (dot) {
      const float ddx = x + 0.5f - dotX, ddy = y + 0.5f - dotY;
      dd = sqrtf(ddx * ddx + ddy * ddy);
      ringCov = clampf(ringR + 0.5f - dd, 0, 1);
    }
    if (cov <= 0 && ringCov <= 0) return -1;
    float cr = bg.r, cg = bg.g, cb = bg.b;
    if (cov > 0) {
      float pr = body.r, pg = body.g, pb = body.b;
      if (eyeMode) {
        const float ec = eyeMode == 2 ? 1.0f : clampf(0.5f - eyeDist(lx, ly), 0, 1);
        pr = mixf(pr, eye.r, ec); pg = mixf(pg, eye.g, ec); pb = mixf(pb, eye.b, ec);
      }
      cr = mixf(cr, pr, cov); cg = mixf(cg, pg, cov); cb = mixf(cb, pb, cov);
    }
    if (ringCov > 0) {  // bg-colored ring cut into the body, then the green dot
      cr = mixf(cr, bg.r, ringCov); cg = mixf(cg, bg.g, ringCov); cb = mixf(cb, bg.b, ringCov);
      const float dc = clampf(dotR + 0.5f - dd, 0, 1);
      cr = mixf(cr, gr, dc); cg = mixf(cg, gg, dc); cb = mixf(cb, gb, dc);
    }
    return pack(cr, cg, cb);
  };

  // Every field is 1-Lipschitz in screen pixels, so one sample at a tile
  // center classifies the whole 8x8 tile.
  constexpr int T = 8;
  constexpr float R = 4 * 1.4143f + 1.0f;
  auto classify = [](float d) { return d > R ? 0 : (d < -R ? 2 : 1); };
  for (int ty = r.y0; ty < r.y1; ty += T) {
    const int ty1 = min(ty + T, r.y1);
    const float mcy = ty + T * 0.5f;
    for (int tx = r.x0; tx < r.x1; tx += T) {
      const int tx1 = min(tx + T, r.x1);
      const float mcx = tx + T * 0.5f;
      const float lx = (mcx - cx) * isx, ly = (mcy - cy) * isy;
      const int bodyMode = classify(sdRoundBox(lx, ly, bw, bh, br) * dscale);
      const bool dot = hypotf(mcx - dotX, mcy - dotY) - ringR < R;
      if (bodyMode == 0 && !dot) continue;
      const int eyeMode = (bodyMode && mcx > eyeX0 - T && mcx < eyeX1 + T && mcy > eyeY0 - T && mcy < eyeY1 + T)
                              ? classify(eyeDist(lx, ly)) : 0;
      if (!dot && bodyMode == 2 && eyeMode != 1) {
        const uint16_t c = eyeMode == 2 ? eye565 : body565;
        for (int y = ty; y < ty1; ++y) {
          uint16_t *row = fb + y * W;
          for (int x = tx; x < tx1; ++x) row[x] = c;
        }
        continue;
      }
      for (int y = ty; y < ty1; ++y) {
        uint16_t *row = fb + y * W;
        for (int x = tx; x < tx1; ++x) {
          const int c = shade(x, y, bodyMode, eyeMode, dot);
          if (c >= 0) row[x] = uint16_t(c);
        }
      }
    }
  }

  if (labelOn) {
    const Bot &b = kBots[botIndex];
    const float a = clampf((labelUntil - now) / 400.0f, 0, 1);
    const RGB l = rgb(b.label);
    canvas.setTextColor(unswap(pack(mixf(bg.r, l.r, a), mixf(bg.g, l.g, a), mixf(bg.b, l.b, a))));
    canvas.setFont(&fonts::FreeSansBold12pt7b);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.drawString(labelText, W / 2, 400);
  }
  if (sleeping) {
    canvas.setFont(&fonts::FreeSansBold12pt7b);
    canvas.setTextDatum(textdatum_t::middle_center);
    const float t = now / 1000.0f;
    for (int i = 0; i < 3; ++i) {
      const float ph = fmodf(t * 0.45f + i / 3.0f, 1.0f);
      const float a = sinf(ph * PI) * 0.8f;
      canvas.setTextColor(unswap(pack(mixf(bg.r, 200, a), mixf(bg.g, 200, a), mixf(bg.b, 210, a))));
      canvas.setTextSize(0.7f + ph * 0.6f);
      canvas.drawString("z", int(cx + bw * 0.7f + ph * 50), int(cy - bh - ph * 70));
    }
    canvas.setTextSize(1);
  }

  prevRect = r;
  fullRedraw = false;
  return dirty;
}

// ---------- input ----------
static void wake() { if (sleeping) applyMood(Neutral); }

static void handleButtons(uint32_t now) {
  auto in = buttons.update(now, M5.BtnA.isPressed(), M5.BtnB.isPressed(),
                           M5.BtnA.wasClicked(), M5.BtnB.wasClicked());
  switch (in) {
    case c25k::Input::Prev: wake(); applyBot(botIndex - 1, true); break;   // left
    case c25k::Input::Next: wake(); applyBot(botIndex + 1, true); break;   // right
    case c25k::Input::Enter: applyMood(sleeping ? Neutral : (moodIndex + 1) % Sleepy); break;
    case c25k::Input::Escape: applyMood(sleeping ? Neutral : Sleepy); break;
    default: break;
  }
}

static bool onBody(float x, float y) {
  float cx = kHomeX + ox.x, cy = kHomeY + oy.x - hop;
  return sdRoundBox((x - cx) / sx.x, (y - cy) / sy.x, hw.x, hh.x, rad.x) < 18;
}

static void handleTouch(uint32_t now) {
  lgfx::touch_point_t raw;
  const bool pressed = M5.Display.getTouchRaw(&raw) > 0;
  if (pressed) {
    auto p = touchcal::applyWarp(touchMap, {double(raw.x), double(raw.y)});
    touchX = clampf(p.x, 0, W - 1); touchY = clampf(p.y, 0, H - 1);
    lastTouchMs = now;
    if (!touchDown) {
      touchDown = true;
      touchStartMs = now; touchStartX = touchX; touchStartY = touchY;
      wake();
      dragging = false;
      grabDX = touchX - (kHomeX + ox.x); grabDY = touchY - (kHomeY + oy.x);
    } else if (!dragging && onBody(touchStartX, touchStartY) &&
               hypotf(touchX - touchStartX, touchY - touchStartY) > 14) {
      dragging = true;
      Serial.println("GB_EVENT drag");
    }
  } else if (touchDown) {
    touchDown = false;
    const bool tap = now - touchStartMs < 350 && hypotf(touchX - touchStartX, touchY - touchStartY) < 14;
    if (dragging) {
      sy.v -= 4.0f; sx.v += 3.5f;
      dragging = false;
    } else if (tap && onBody(touchX, touchY)) {
      startHop();
      Serial.println("GB_EVENT hop");
    } else if (tap) {
      float dx = touchX - kHomeX, dy = touchY - kHomeY, len = hypotf(dx, dy);
      if (len > 1) saccade(dx / len, dy / len * 0.8f);
      nextGlance = now + 1800;
      Serial.printf("GB_EVENT look x=%d y=%d\n", int(touchX), int(touchY));
    }
  }
}

static void handleImu(uint32_t now) {
  if (!imuOk || !M5.Imu.update()) return;
  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);
  // Axis mapping for the Stopwatch face still needs on-device confirmation.
  tiltX = mixf(tiltX, -ax, 0.15f);
  tiltY = mixf(tiltY, ay, 0.15f);
  const float mag = sqrtf(ax * ax + ay * ay + az * az);
  if (mag > 2.2f && !airborne && !sleeping) startHop();
  if (now - lastImuLog > 5000) {
    lastImuLog = now;
    Serial.printf("GB_IMU ax=%.2f ay=%.2f az=%.2f tilt=%.2f,%.2f\n", ax, ay, az, tiltX, tiltY);
  }
}

static void dumpFrame() {
  Serial.printf("GB_SHOT %d %d\n", W, H);
  Serial.flush();
  const uint8_t *p = reinterpret_cast<const uint8_t *>(fb);
  const size_t total = size_t(W) * H * 2;
  for (size_t off = 0; off < total; off += 4096) Serial.write(p + off, min<size_t>(4096, total - off));
  Serial.flush();
  Serial.println("\nGB_SHOT_END");
}

static int moodByName(const char *name) {
  for (int i = 0; i < kMoodCount; ++i) if (!strcmp(kMoods[i].name, name)) return i;
  return -1;
}

static void handleSerial() {
  static char line[64];
  static size_t n = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      line[n] = 0;
      n = 0;
      if (!strcmp(line, "hop")) startHop();
      else if (!strcmp(line, "next")) applyBot(botIndex + 1, true);
      else if (!strcmp(line, "prev")) applyBot(botIndex - 1, true);
      else if (!strcmp(line, "blink")) blink(millis());
      else if (!strcmp(line, "shot")) dumpFrame();
      else if (!strncmp(line, "mood ", 5)) {
        const int m = moodByName(line + 5);
        if (m >= 0) applyMood(m); else Serial.printf("GB_ERROR unknown_mood=%s\n", line + 5);
      } else if (!strncmp(line, "look ", 5)) {
        float x, y;
        if (sscanf(line + 5, "%f %f", &x, &y) == 2) { saccade(x, y); nextGlance = millis() + 5000; }
      }
    } else if (n < sizeof(line) - 1) line[n++] = c;
  }
}

// ---------- setup / loop ----------
void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  cfg.internal_imu = true;
  cfg.internal_rtc = false;
  cfg.internal_mic = false;
  cfg.internal_spk = false;
  cfg.fallback_board = m5::board_t::board_M5StopWatch;
  M5.begin(cfg);
  auto *panel = M5.Display.getPanel();
  auto pc = panel->config();
  pc.panel_width = 468; pc.panel_height = 466; pc.offset_x = 6; pc.offset_y = 0;
  panel->config(pc);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(255);
  Serial.printf("GB_HARDWARE board=%d expected=%d size=%dx%d touch=%d psram=%u\n",
                int(M5.getBoard()), int(m5::board_t::board_M5StopWatch),
                M5.Display.width(), M5.Display.height(), M5.Touch.isEnabled(), ESP.getPsramSize());

  const bool mapOk = loadTouchMap();
  Serial.printf("GB_TOUCHMAP ok=%d version=%u generation=%u\n", mapOk, touchMapVersion, touchMapGeneration);
  imuOk = M5.Imu.isEnabled();

  canvas.setPsram(true);
  canvas.setColorDepth(16);
  fb = static_cast<uint16_t *>(canvas.createSprite(W, H));
  if (!fb) { Serial.println("GB_FATAL canvas"); while (true) delay(1000); }

  const Bot &b = kBots[0];
  hw.init(b.hw, 170, 11); hh.init(b.hh, 170, 11); rad.init(b.rad, 170, 11);
  const RGB c = rgb(b.body), e = rgb(b.eye), g = rgb(b.bg);
  bodyR.init(c.r, 60, 16); bodyG.init(c.g, 60, 16); bodyB.init(c.b, 60, 16);
  eyeR.init(e.r, 60, 16); eyeG.init(e.g, 60, 16); eyeB.init(e.b, 60, 16);
  bgR.init(g.r, 40, 13); bgG.init(g.g, 40, 13); bgB.init(g.b, 40, 13);
  sx.init(1, 260, 9); sy.init(1, 260, 9);
  ox.init(0, 70, 11); oy.init(0, 70, 11);               // soft: body lags the eyes
  gx.init(0, 900, 36); gy.init(0, 900, 36);             // snappy darts, ~8% overshoot
  eyeOpen.init(0.08f, 1100, 34);                        // blinks bounce open
  const Mood &m = kMoods[Neutral];
  eLen.init(m.len, 260, 20); eRad.init(m.rad, 260, 20); eArc.init(m.arc, 200, 22);
  eTilt.init(m.tilt, 240, 20); eSpread.init(m.spread, 240, 20); eSpin.init(m.spin, 160, 22);
  sx.x = 1.35f; sy.x = 0.65f;  // entrance: pop in from a squashed blob, eyes opening

  const uint32_t now = millis();
  nextBlink = now + 1500; nextGlance = now + 800; nextIdleHop = now + 6000;
  showLabel(b.name);
  Serial.println("GB_READY");
}

void loop() {
  static uint32_t last = millis(), fpsStart = millis(), frames = 0, renderUs = 0, busyUs = 0;
  M5.update();
  const uint32_t now = millis();
  const float dt = clampf((now - last) / 1000.0f, 0.001f, 0.05f);
  last = now;

  handleButtons(now);
  handleTouch(now);
  handleImu(now);
  handleSerial();
  tick(dt, now);

  const uint32_t t0 = micros();
  const Rect d = render(now);
  renderUs += micros() - t0;
  M5.Display.setClipRect(d.x0, d.y0, d.x1 - d.x0, d.y1 - d.y0);
  canvas.pushSprite(0, 0);
  M5.Display.clearClipRect();
  busyUs += micros() - t0;

  if (++frames, now - fpsStart >= 5000) {
    Serial.printf("GB_FPS fps=%.1f frame_ms=%.1f render_ms=%.1f heap=%u stack_free=%u\n",
                  frames * 1000.0f / (now - fpsStart), busyUs / 1000.0f / frames, renderUs / 1000.0f / frames,
                  ESP.getFreeHeap(), uxTaskGetStackHighWaterMark(nullptr));
    frames = 0; renderUs = 0; busyUs = 0; fpsStart = now;
  }
}
