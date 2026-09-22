// ESPtember Day 10 — Buttons, Buzzer, Haptics
// The StopWatch kit has no touchscreen. This lesson teaches the
// replacement grammar: physical buttons with feedback you can feel and
// hear without looking. Five gestures — click A, click B, hold A,
// hold B, chord — each with its own tone and vibration, so the device
// is operable from a pocket.
#include <M5Unified.h>

// --- Gesture grammar --------------------------------------------------
// M5Unified debounces for us and reports clicks on release and holds at
// a time threshold. The only part it can't decide alone is the chord:
// both buttons down together must NOT also fire two clicks. The rule
// from day 12 applies: a gesture owns its buttons until both release.

enum class Gesture { None, ClickA, ClickB, HoldA, HoldB, Chord };

static const char *gestureName(Gesture g) {
  switch (g) {
    case Gesture::ClickA: return "CLICK A";
    case Gesture::ClickB: return "CLICK B";
    case Gesture::HoldA: return "HOLD A";
    case Gesture::HoldB: return "HOLD B";
    case Gesture::Chord: return "CHORD";
    default: return "--";
  }
}

class Grammar {
 public:
  Gesture update() {
    const bool a = M5.BtnA.isPressed(), b = M5.BtnB.isPressed();
    if (locked_) {           // chord fired; swallow everything
      if (!a && !b) locked_ = false;
      return Gesture::None;
    }
    if (a && b) {            // both down = chord, immediately
      locked_ = true;
      return Gesture::Chord;
    }
    if (M5.BtnA.wasHold()) { holdFired_ = true; return Gesture::HoldA; }
    if (M5.BtnB.wasHold()) { holdFired_ = true; return Gesture::HoldB; }
    if (M5.BtnA.wasClicked()) return holdConsumed() ? Gesture::None : Gesture::ClickA;
    if (M5.BtnB.wasClicked()) return holdConsumed() ? Gesture::None : Gesture::ClickB;
    if (!a && !b) holdFired_ = false;
    return Gesture::None;
  }

 private:
  bool holdConsumed() {      // a release after a hold is not a click
    if (!holdFired_) return false;
    holdFired_ = false;
    return true;
  }
  bool locked_ = false;
  bool holdFired_ = false;
};

// --- Feedback ---------------------------------------------------------
// Every gesture maps to one tone and one vibration. The point of the
// mapping is discrimination: with the device in a pocket, each gesture
// must feel different. Clicks are short and high; holds are long and
// low; the chord is both motors of communication at once.

//   gesture            tone         vibration
static const uint16_t cueHz[6]    = {0, 880, 660, 440, 330, 1046};
static const uint16_t cueToneMs[6]= {0,  80,  80, 220, 220,  300};
static const uint8_t  cueVibe[6]  = {0, 120, 120, 200, 200,  255};
static const uint16_t cueVibeMs[6]= {0,  60,  60, 220, 220,  320};

static uint32_t vibeOffAt = 0;

static void playCue(Gesture g) {
  const int i = (int)g;
  if (cueToneMs[i]) M5.Speaker.tone(cueHz[i], cueToneMs[i], 0, true);
  if (cueVibeMs[i]) {
    M5.Power.setVibration(cueVibe[i]);
    vibeOffAt = millis() + cueVibeMs[i];
  }
}

static void updateVibe() {
  if (vibeOffAt && int32_t(millis() - vibeOffAt) >= 0) {
    M5.Power.setVibration(0);
    vibeOffAt = 0;
  }
}

// --- Display ----------------------------------------------------------
// 466x466 round AMOLED. Big center readout of the last gesture, a
// counter per gesture around it. Full redraw on event only — nothing
// animates, so nothing needs a frame loop.

static int counts[6] = {0};
static Gesture lastGesture = Gesture::None;

static void render() {
  M5.Display.startWrite();
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);

  M5.Display.setTextColor(0xFD20 /* orange */, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.drawString("DAY 10", 233, 90);

  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(4);
  M5.Display.drawString(gestureName(lastGesture), 233, 210);

  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  char line[48];
  snprintf(line, sizeof(line), "A:%d  B:%d", counts[(int)Gesture::ClickA],
           counts[(int)Gesture::ClickB]);
  M5.Display.drawString(line, 233, 300);
  snprintf(line, sizeof(line), "holdA:%d  holdB:%d  chord:%d",
           counts[(int)Gesture::HoldA], counts[(int)Gesture::HoldB],
           counts[(int)Gesture::Chord]);
  M5.Display.drawString(line, 233, 340);
  M5.Display.endWrite();
}

// --- Serial status (development aid) -----------------------------------
static void reportStatus() {
  Serial.printf("D10_STATUS last=%s clickA=%d clickB=%d holdA=%d holdB=%d chord=%d\n",
                gestureName(lastGesture), counts[(int)Gesture::ClickA],
                counts[(int)Gesture::ClickB], counts[(int)Gesture::HoldA],
                counts[(int)Gesture::HoldB], counts[(int)Gesture::Chord]);
}

static Grammar grammar;

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
  Serial.println("D10_READY");
  reportStatus();
}

void loop() {
  M5.update();
  const Gesture g = grammar.update();
  if (g != Gesture::None) {
    lastGesture = g;
    counts[(int)g]++;
    playCue(g);
    render();
    reportStatus();
  }
  updateVibe();
  if (Serial.available() && Serial.read() == 's') reportStatus();
  delay(5);
}
