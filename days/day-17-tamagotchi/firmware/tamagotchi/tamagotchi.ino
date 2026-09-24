// ESPtember Day 17 — Tamagotchi
// A clean-room virtual pet in the 1996 P1 tradition: hunger and
// happiness as four hearts each, feeding, play, poop, sleep, and a
// creature that ages IN REAL TIME — including while powered off. On
// boot, the elapsed hours since last seen are replayed against the
// meters and the life stage. Art is original: a 16x16 1-bit creature
// drawn as fat pixels inside the round face. Two buttons run the whole
// toy: A cycles the action, B confirms — the original's three-button
// interface minus one, solved by a wrapping menu.
#include <M5Unified.h>
#include <Preferences.h>

// --- Life stages ---------------------------------------------------------
// Thresholds are minutes of age. The egg hatches in five minutes — the
// 1996 original's first-session payoff — then childhood at one day and
// adulthood at three.
enum Stage : uint8_t { EGG, BABY, CHILD, ADULT, DEAD };
static const uint32_t STAGE_AT_MINUTES[] = {0, 5, 1440, 4320};
static const char *STAGE_NAME[] = {"EGG", "BABY", "CHILD", "ADULT", "GONE"};

// --- Pet state -------------------------------------------------------------
// Meters run 0..4 (hearts). Decay: hunger loses a heart every 4 hours,
// happiness every 6. A poop appears every 5 waking hours and parks a
// happiness penalty until cleaned. Death: 24 consecutive hours with
// hunger empty.
struct Pet {
  uint32_t bornAt = 0;      // epoch seconds
  uint32_t lastSeen = 0;    // epoch seconds at last save
  uint8_t hunger = 4, happy = 4;
  uint8_t poops = 0;
  uint8_t careMistakes = 0;
  uint32_t hungerZeroHours = 0;
  uint8_t stage = EGG;
  // fractional-hour accumulators so short sessions still add up
  uint16_t hungerMin = 0, happyMin = 0, poopMin = 0;
};
static Pet pet;
static Preferences prefs;

static uint32_t nowEpoch() {
  auto t = M5.Rtc.getDateTime();
  struct tm tmv = {};
  tmv.tm_year = t.date.year - 1900;
  tmv.tm_mon = t.date.month - 1;
  tmv.tm_mday = t.date.date;
  tmv.tm_hour = t.time.hours;
  tmv.tm_min = t.time.minutes;
  tmv.tm_sec = t.time.seconds;
  return (uint32_t)mktime(&tmv);
}

static uint32_t ageMinutes() { return (nowEpoch() - pet.bornAt) / 60; }
static uint32_t ageHours() { return (nowEpoch() - pet.bornAt) / 3600; }

static void savePet() {
  pet.lastSeen = nowEpoch();
  prefs.putBytes("pet", &pet, sizeof(pet));
}

// Advance the simulation by whole minutes. This one function serves
// both the live loop (one minute at a time) and the boot replay
// (potentially thousands of minutes) — offline aging is not a special
// case, it's just a bigger argument.
static void simulate(uint32_t minutes) {
  if (pet.stage == DEAD) return;
  for (uint32_t m = 0; m < minutes; m++) {
    if (pet.stage != EGG) {
      if (++pet.hungerMin >= 240) { // 4 hours per heart
        pet.hungerMin = 0;
        if (pet.hunger) pet.hunger--;
        else pet.careMistakes++;
      }
      if (++pet.happyMin >= 360) { // 6 hours per heart
        pet.happyMin = 0;
        uint8_t drop = pet.poops ? 2 : 1; // filth accelerates misery
        pet.happy = pet.happy > drop ? pet.happy - drop : 0;
        if (!pet.happy) pet.careMistakes++;
      }
      if (++pet.poopMin >= 300 && pet.poops < 4) { // 5 hours
        pet.poopMin = 0;
        pet.poops++;
      }
      if (pet.hunger == 0) {
        if (++pet.hungerZeroHours >= 24 * 60) { pet.stage = DEAD; return; }
      } else {
        pet.hungerZeroHours = 0;
      }
    }
  }
  // Stage from age, never backwards.
  const uint32_t age = ageMinutes();
  uint8_t stage = EGG;
  for (int s = ADULT; s >= 0; s--)
    if (age >= STAGE_AT_MINUTES[s]) { stage = s; break; }
  if (stage > pet.stage && pet.stage != DEAD) pet.stage = stage;
}

// --- Art -----------------------------------------------------------------
// Original creatures on a 16x16 1-bit grid, one uint16_t per row, drawn
// as 14 px fat pixels. No Bandai sprites — these are ours.
static const uint16_t ART_EGG[16] = {
    0x0000, 0x03C0, 0x0FF0, 0x1FF8, 0x3FFC, 0x3FFC, 0x7FFE, 0x7FFE,
    0x7FFE, 0x7FFE, 0x7FFE, 0x3FFC, 0x3FFC, 0x1FF8, 0x0FF0, 0x03C0};
static const uint16_t ART_BABY[16] = {
    0x0000, 0x0000, 0x0000, 0x03C0, 0x0FF0, 0x1FF8, 0x1B58, 0x1FF8,
    0x1FF8, 0x1DB8, 0x1E78, 0x0FF0, 0x03C0, 0x0240, 0x0660, 0x0000};
static const uint16_t ART_CHILD[16] = {
    0x0000, 0x0C30, 0x0C30, 0x0FF0, 0x1FF8, 0x3FFC, 0x36DC, 0x3FFC,
    0x3FFC, 0x3BDC, 0x3C3C, 0x1FF8, 0x0FF0, 0x0C30, 0x1C38, 0x0000};
static const uint16_t ART_ADULT[16] = {
    0x1008, 0x381C, 0x1FF8, 0x3FFC, 0x7FFE, 0x7FFE, 0x6DB6, 0x7FFE,
    0x7FFE, 0x77EE, 0x783E, 0x3FFC, 0x1FF8, 0x1C38, 0x3C3C, 0x0000};
static const uint16_t ART_DEAD[16] = {
    0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x7FFE, 0x6666, 0x7FFE, 0x7FFE,
    0x6DB6, 0x7FFE, 0x3FFC, 0x1FF8, 0x0000, 0x7FFE, 0x7FFE, 0x0000};

static const uint16_t *artFor(uint8_t stage) {
  switch (stage) {
    case EGG: return ART_EGG;
    case BABY: return ART_BABY;
    case CHILD: return ART_CHILD;
    case ADULT: return ART_ADULT;
    default: return ART_DEAD;
  }
}

// --- Menu -------------------------------------------------------------------
enum Action : uint8_t { FEED, SNACK, PLAY, CLEAN, ACTION_COUNT };
static const char *ACTION_NAME[] = {"FEED", "SNACK", "PLAY", "CLEAN"};
static uint8_t selected = FEED;
static char toast[24] = "";
static uint32_t toastUntil = 0;

static void say(const char *msg) {
  snprintf(toast, sizeof(toast), "%s", msg);
  toastUntil = millis() + 2500;
}

static void doAction() {
  if (pet.stage == EGG) { say("STILL AN EGG"); return; }
  if (pet.stage == DEAD) { say("..."); return; }
  switch (selected) {
    case FEED: // meal: fills hunger
      pet.hunger = 4;
      pet.hungerMin = 0;
      M5.Speaker.tone(660, 80, 0, true);
      say("YUM");
      break;
    case SNACK: // snack: one happiness heart, the junk-food bargain
      if (pet.happy < 4) pet.happy++;
      M5.Speaker.tone(880, 80, 0, true);
      say("SWEET!");
      break;
    case PLAY: // guess: creature picks a side, B was your guess
      if ((esp_random() & 1) == 0) {
        if (pet.happy < 4) pet.happy++;
        M5.Speaker.tone(1046, 120, 0, true);
        say("IT LIKED THAT");
      } else {
        M5.Speaker.tone(330, 120, 0, true);
        say("NOT IN THE MOOD");
      }
      break;
    case CLEAN:
      if (pet.poops) { pet.poops = 0; say("ALL CLEAN"); }
      else say("ALREADY CLEAN");
      M5.Speaker.tone(440, 80, 0, true);
      break;
  }
  savePet();
}

// --- Display -------------------------------------------------------------------
static void drawHearts(int x, int y, uint8_t filled) {
  for (int i = 0; i < 4; i++) {
    const int cx = x + i * 30;
    if (i < filled) M5.Display.fillCircle(cx, y, 9, 0xF800);
    else M5.Display.drawCircle(cx, y, 9, TFT_DARKGRAY);
  }
}

static void render() {
  M5.Display.startWrite();
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);

  M5.Display.setTextSize(2);
  M5.Display.setTextColor(0xFD20, TFT_BLACK);
  char head[32];
  snprintf(head, sizeof(head), "%s  %uh", STAGE_NAME[pet.stage],
           (unsigned)ageHours());
  M5.Display.drawString(head, 233, 64);

  // The creature: 16x16 grid, 14px pixels, centered.
  const uint16_t *art = artFor(pet.stage);
  const int px = 14, ox = 233 - 8 * px, oy = 210 - 8 * px;
  const uint16_t color = pet.stage == DEAD ? TFT_DARKGRAY : 0x07E0;
  for (int row = 0; row < 16; row++)
    for (int col = 0; col < 16; col++)
      if (art[row] & (0x8000 >> col))
        M5.Display.fillRect(ox + col * px, oy + row * px, px - 1, px - 1,
                            color);

  // Poops line up beside the creature.
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(0x8283, TFT_BLACK);
  for (int i = 0; i < pet.poops; i++)
    M5.Display.drawString("~", 380, 150 + i * 30);

  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.drawString("HUNGRY", 160, 320);
  drawHearts(255, 320, pet.hunger);
  M5.Display.drawString("HAPPY", 160, 352);
  drawHearts(255, 352, pet.happy);

  // Menu: A cycles, B does.
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  char menu[32];
  snprintf(menu, sizeof(menu), "< %s >", ACTION_NAME[selected]);
  M5.Display.drawString(menu, 233, 400);

  if (millis() < toastUntil) {
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(0xFD20, TFT_BLACK);
    M5.Display.drawString(toast, 233, 110);
  }
  M5.Display.endWrite();
}

static void reportStatus() {
  Serial.printf(
      "D16_STATUS stage=%s age_h=%u hunger=%d happy=%d poops=%d mistakes=%d\n",
      STAGE_NAME[pet.stage], (unsigned)ageHours(), pet.hunger, pet.happy,
      pet.poops, pet.careMistakes);
}

static uint32_t lastMinuteTick = 0;

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

  prefs.begin("day16", false);
  if (prefs.getBytes("pet", &pet, sizeof(pet)) != sizeof(pet) ||
      pet.bornAt == 0) {
    pet = Pet();
    pet.bornAt = pet.lastSeen = nowEpoch();
    savePet();
    say("AN EGG APPEARED");
  } else {
    // Offline aging: replay every minute we were powered off.
    const uint32_t away = nowEpoch() - pet.lastSeen;
    if (away > 60) simulate(away / 60);
    savePet();
    if (pet.stage == DEAD) say("SOMETHING HAPPENED");
  }

  render();
  Serial.println("D16_READY");
  reportStatus();
  lastMinuteTick = millis();
}

void loop() {
  M5.update();
  bool dirty = false;

  if (M5.BtnA.wasClicked()) {
    selected = (selected + 1) % ACTION_COUNT;
    M5.Speaker.tone(523, 40, 0, true);
    dirty = true;
  }
  if (M5.BtnB.wasClicked()) {
    doAction();
    reportStatus();
    dirty = true;
  }
  // Chord (both held): restart after death — the circle of life.
  if (pet.stage == DEAD && M5.BtnA.isPressed() && M5.BtnB.isPressed()) {
    pet = Pet();
    pet.bornAt = pet.lastSeen = nowEpoch();
    savePet();
    say("AN EGG APPEARED");
    reportStatus();
    dirty = true;
  }

  // Live simulation: one simulated minute per real minute.
  if (millis() - lastMinuteTick >= 60000) {
    lastMinuteTick += 60000;
    const uint8_t before = pet.stage;
    simulate(1);
    savePet();
    if (pet.stage != before) {
      M5.Speaker.tone(1319, 200, 0, true);
      say("IT GREW!");
      reportStatus();
    }
    dirty = true;
  }

  if (Serial.available() && Serial.read() == 's') reportStatus();
  if (dirty || millis() < toastUntil) render();
  delay(10);
}
