// ESPtember Day 14 — Morse Code Practicer
// A straight key on the crown, ITU-R M.1677-1 timing, live decode.
// Press A and the sidetone sings; release and the press length decides
// dit or dah. Gaps decide letters and words. The screen shows both the
// raw symbol stream and the characters it becomes. B taps clear, B held
// cycles keying speed. Everything here is the timing discipline from
// days 10-11 pointed at a 180-year-old protocol.
#include <M5Unified.h>

// --- ITU timing ---------------------------------------------------------
// One unit is the dit. Everything else is integer multiples (ITU-R
// M.1677-1 §2): dah = 3 units, intra-character gap = 1, letter gap = 3,
// word gap = 7. Unit length derives from words-per-minute via the PARIS
// convention: unit ms = 1200 / WPM.

static const uint8_t wpmSteps[] = {5, 10, 15, 20};
static int wpmIndex = 1; // default 10 WPM: 120 ms unit

static uint32_t unitMs() { return 1200 / wpmSteps[wpmIndex]; }

// A press shorter than 2 units is a dit; longer is a dah. The midpoint
// splits the 1-unit dit from the 3-unit dah with equal tolerance both
// ways.
static bool isDah(uint32_t pressMs) { return pressMs >= 2 * unitMs(); }

// --- Code table ----------------------------------------------------------
// Symbols encode left-to-right; '.' = dit, '-' = dah.
struct MorseEntry { const char *code; char ch; };
static const MorseEntry MORSE[] = {
    {".-", 'A'},    {"-...", 'B'},  {"-.-.", 'C'},  {"-..", 'D'},
    {".", 'E'},     {"..-.", 'F'},  {"--.", 'G'},   {"....", 'H'},
    {"..", 'I'},    {".---", 'J'},  {"-.-", 'K'},   {".-..", 'L'},
    {"--", 'M'},    {"-.", 'N'},    {"---", 'O'},   {".--.", 'P'},
    {"--.-", 'Q'},  {".-.", 'R'},   {"...", 'S'},   {"-", 'T'},
    {"..-", 'U'},   {"...-", 'V'},  {".--", 'W'},   {"-..-", 'X'},
    {"-.--", 'Y'},  {"--..", 'Z'},  {"-----", '0'}, {".----", '1'},
    {"..---", '2'}, {"...--", '3'}, {"....-", '4'}, {".....", '5'},
    {"-....", '6'}, {"--...", '7'}, {"---..", '8'}, {"----.", '9'},
};

static char lookup(const char *code) {
  for (auto &entry : MORSE)
    if (!strcmp(entry.code, code)) return entry.ch;
  return '?';
}

// --- Keyer state ------------------------------------------------------------
static char symbolBuf[8];   // current character's dits and dahs
static int symbolLen = 0;
static char text[64];       // decoded message
static int textLen = 0;
static uint32_t pressStart = 0;
static uint32_t lastRelease = 0;
static bool keyDown = false;
static bool letterClosed = true;
static bool wordClosed = true;

static void appendText(char c) {
  if (textLen + 1 >= (int)sizeof(text)) { // scroll: drop oldest half
    memmove(text, text + sizeof(text) / 2, textLen - sizeof(text) / 2);
    textLen -= sizeof(text) / 2;
  }
  text[textLen++] = c;
  text[textLen] = 0;
}

static void closeLetter() {
  if (!symbolLen) return;
  symbolBuf[symbolLen] = 0;
  appendText(lookup(symbolBuf));
  symbolLen = 0;
  letterClosed = true;
}

// --- Display -----------------------------------------------------------------
static void render() {
  M5.Display.startWrite();
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);

  M5.Display.setTextColor(0xFD20, TFT_BLACK);
  M5.Display.setTextSize(2);
  char header[24];
  snprintf(header, sizeof(header), "MORSE  %u WPM", wpmSteps[wpmIndex]);
  M5.Display.drawString(header, 233, 80);

  // The symbol in progress, drawn big — dits and dahs as typed.
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(6);
  symbolBuf[symbolLen] = 0;
  M5.Display.drawString(symbolLen ? symbolBuf : "", 233, 170);

  // Decoded text, most recent tail.
  M5.Display.setTextSize(3);
  const char *tail = textLen > 14 ? text + textLen - 14 : text;
  M5.Display.drawString(tail, 233, 270);

  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  M5.Display.drawString("A: key   B: clear", 233, 350);
  M5.Display.drawString("B hold: speed", 233, 382);
  M5.Display.endWrite();
}

static void reportStatus() {
  symbolBuf[symbolLen] = 0;
  Serial.printf("D13_STATUS wpm=%u unit=%u symbol=%s text=%s\n",
                wpmSteps[wpmIndex], (unsigned)unitMs(), symbolBuf, text);
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
  text[0] = 0;
  render();
  Serial.println("D13_READY");
  reportStatus();
}

void loop() {
  M5.update();
  bool dirty = false;
  const uint32_t now = millis();

  // The straight key. Sidetone runs exactly as long as the press —
  // 600 Hz, the traditional CW pitch.
  if (M5.BtnA.wasPressed()) {
    keyDown = true;
    pressStart = now;
    letterClosed = wordClosed = false;
    M5.Speaker.tone(600, 10000, 0, true); // stopped on release
  }
  if (keyDown && M5.BtnA.wasReleased()) {
    keyDown = false;
    M5.Speaker.stop();
    const uint32_t pressMs = now - pressStart;
    if (symbolLen < (int)sizeof(symbolBuf) - 1)
      symbolBuf[symbolLen++] = isDah(pressMs) ? '-' : '.';
    lastRelease = now;
    dirty = true;
  }

  // Gap decoding: 3 units closes the letter, 7 closes the word.
  if (!keyDown && lastRelease) {
    const uint32_t gap = now - lastRelease;
    if (!letterClosed && gap >= 3 * unitMs()) {
      closeLetter();
      reportStatus();
      dirty = true;
    }
    if (!wordClosed && letterClosed && gap >= 7 * unitMs()) {
      if (textLen && text[textLen - 1] != ' ') appendText(' ');
      wordClosed = true;
      dirty = true;
    }
  }

  // B: clear on tap, speed on hold.
  if (M5.BtnB.wasClicked()) {
    textLen = 0;
    text[0] = 0;
    symbolLen = 0;
    dirty = true;
    reportStatus();
  }
  if (M5.BtnB.wasHold()) {
    wpmIndex = (wpmIndex + 1) % (int)(sizeof(wpmSteps) / sizeof(wpmSteps[0]));
    M5.Speaker.tone(880, 80, 0, true);
    dirty = true;
    reportStatus();
  }

  if (Serial.available() && Serial.read() == 's') reportStatus();
  if (dirty) render();
  delay(2);
}
