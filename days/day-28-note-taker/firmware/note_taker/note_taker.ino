// ESPtember Day 28 — Note Taker
// A voice recorder that writes: hold the crown and speak (up to 30
// seconds into PSRAM), release, and the clip goes to Deepgram for
// transcription; the text files itself LOCALLY — a ring of twenty notes
// in NVS, browsable on the second pusher. No keyboard ever existed and
// none was missed. Cloud backends (Notion, Telegram, Memos, your own
// Worker) are the README's extended options.
// Hold BOTH pushers ~2 s at any time to forget Wi-Fi and reopen the
// setup portal.
//
// Provisioning: with no saved Wi-Fi the board becomes the setup page —
// join "esptember-setup" from a phone; the form collects Wi-Fi and the
// Deepgram key. Serial stays available as the power-user path:
//   wifi SSID PASS / dgkey KEY / forget
#include <M5Unified.h>
#include "wifi_portal.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#define SAMPLE_RATE 8000
#define MAX_SECONDS 30
#define MAX_SAMPLES (SAMPLE_RATE * MAX_SECONDS)
#define CHUNK 512

static Preferences wifiPrefs, notePrefs;
static int16_t *recording; // PSRAM
static size_t recorded = 0;

enum class State { Idle, Recording, Transcribing, Saving, Done, Error };
static State state = State::Idle;
static char lastNote[160] = "";
static char statusLine[48] = "";
static int notesSaved = 0;

// --- Deepgram prerecorded -------------------------------------------------------
static bool transcribe(String &text) {
  const String key = notePrefs.getString("dgkey", "");
  if (!key.length()) { snprintf(statusLine, sizeof(statusLine), "no dgkey"); return false; }
  HTTPClient http;
  http.begin("https://api.deepgram.com/v1/listen?encoding=linear16&"
             "sample_rate=8000&channels=1&smart_format=true");
  http.addHeader("Authorization", "Token " + key);
  http.addHeader("Content-Type", "application/octet-stream");
  http.setTimeout(30000);
  const int code =
      http.POST((uint8_t *)recording, recorded * sizeof(int16_t));
  if (code != 200) {
    snprintf(statusLine, sizeof(statusLine), "deepgram HTTP %d", code);
    http.end();
    return false;
  }
  JsonDocument filter;
  filter["results"]["channels"][0]["alternatives"][0]["transcript"] = true;
  JsonDocument doc;
  const DeserializationError err = deserializeJson(
      doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) { snprintf(statusLine, sizeof(statusLine), "bad JSON"); return false; }
  text = doc["results"]["channels"][0]["alternatives"][0]["transcript"]
             .as<String>();
  return text.length() > 0;
}

// --- Local note store ---------------------------------------------------------
// A ring of twenty notes in NVS: note0..note19 plus a cursor. Local
// first — the notes survive power loss and browse on the B pusher; a
// cloud backend is one function away when wanted.
#define NOTE_SLOTS 20

static int noteCursor = 0; // next slot to write
static int noteTotal = 0;  // lifetime count (caps display at slots)
static int browseIndex = -1;

static void notesLoad() {
  noteCursor = notePrefs.getInt("cursor", 0);
  noteTotal = notePrefs.getInt("total", 0);
}

static void noteKey(char *out, size_t n, int slot) {
  snprintf(out, n, "note%d", slot);
}

static bool saveNoteLocal(const String &text) {
  char key[12];
  noteKey(key, sizeof(key), noteCursor % NOTE_SLOTS);
  if (!notePrefs.putString(key, text)) {
    snprintf(statusLine, sizeof(statusLine), "NVS write failed");
    return false;
  }
  noteCursor = (noteCursor + 1) % NOTE_SLOTS;
  noteTotal++;
  notePrefs.putInt("cursor", noteCursor);
  notePrefs.putInt("total", noteTotal);
  return true;
}

static String readNote(int back) { // back=0: newest
  const int stored = noteTotal < NOTE_SLOTS ? noteTotal : NOTE_SLOTS;
  if (back < 0 || back >= stored) return "";
  char key[12];
  noteKey(key, sizeof(key), (noteCursor - 1 - back + 2 * NOTE_SLOTS) % NOTE_SLOTS);
  return notePrefs.getString(key, "");
}

// --- Display -----------------------------------------------------------------------
static M5Canvas canvas(&M5.Display);

static void render() {
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(0xFD20, TFT_BLACK);
  canvas.setTextSize(2);
  canvas.drawString("NOTES", 233, 60);

  canvas.setTextSize(3);
  switch (state) {
    case State::Idle:
      canvas.setTextColor(WiFi.status() == WL_CONNECTED ? TFT_WHITE
                                                        : TFT_DARKGRAY,
                          TFT_BLACK);
      canvas.drawString(WiFi.status() == WL_CONNECTED ? "hold A to speak"
                                                      : "no Wi-Fi",
                        233, 200);
      break;
    case State::Recording: {
      canvas.fillCircle(233, 180, 26, 0xF800);
      char t[16];
      snprintf(t, sizeof(t), "%u s", (unsigned)(recorded / SAMPLE_RATE));
      canvas.setTextColor(TFT_WHITE, TFT_BLACK);
      canvas.drawString(t, 233, 250);
      break;
    }
    case State::Transcribing:
      canvas.setTextColor(TFT_WHITE, TFT_BLACK);
      canvas.drawString("transcribing...", 233, 200);
      break;
    case State::Saving:
      canvas.setTextColor(TFT_WHITE, TFT_BLACK);
      canvas.drawString("saving...", 233, 200);
      break;
    case State::Done:
      canvas.setTextColor(0x07E0, TFT_BLACK);
      canvas.drawString("SAVED", 233, 140);
      break;
    case State::Error:
      canvas.setTextColor(0xF800, TFT_BLACK);
      canvas.drawString("FAILED", 233, 140);
      break;
  }

  // The last note (or the failure reason) wraps below.
  canvas.setTextSize(2);
  canvas.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  const char *detail = state == State::Error ? statusLine : lastNote;
  char buf[160];
  strlcpy(buf, detail, sizeof(buf));
  int y = 210;
  if (state == State::Done || state == State::Error) {
    char *word = strtok(buf, " ");
    char lineBuf[30] = "";
    while (word && y < 380) {
      if (strlen(lineBuf) + strlen(word) + 1 < 28) {
        if (lineBuf[0]) strlcat(lineBuf, " ", sizeof(lineBuf));
        strlcat(lineBuf, word, sizeof(lineBuf));
      } else {
        canvas.drawString(lineBuf, 233, y);
        y += 28;
        strlcpy(lineBuf, word, sizeof(lineBuf));
      }
      word = strtok(NULL, " ");
    }
    if (lineBuf[0]) canvas.drawString(lineBuf, 233, y);
  }

  char foot[32];
  snprintf(foot, sizeof(foot), "%d notes filed", notesSaved);
  canvas.drawString(foot, 233, 420);
  canvas.pushSprite(0, 0);
}

static void reportStatus() {
  Serial.printf("D28_STATUS state=%d wifi=%d saved=%d last=%s status=%s\n",
                (int)state, WiFi.status() == WL_CONNECTED, notesSaved,
                lastNote, statusLine);
}

// --- Serial provisioning -----------------------------------------------------------
static void handleSerial() {
  static char line[256];
  static int len = 0;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\n' || c == '\r') {
      line[len] = 0;
      len = 0;
      char a[128], b[128];
      if (sscanf(line, "wifi %63s %63s", a, b) == 2) {
        wifiPrefs.putString("ssid", a);
        wifiPrefs.putString("pass", b);
        Serial.printf("D28_SAVED wifi=%s\n", a);
        ESP.restart();
      } else if (sscanf(line, "dgkey %127s", a) == 1) {
        notePrefs.putString("dgkey", a);
        Serial.println("D28_SAVED dgkey");
      } else if (!strcmp(line, "forget")) {
        wifiPrefs.clear();
        Serial.println("D28_FORGOT");
        ESP.restart();
      } else if (!strcmp(line, "s")) {
        reportStatus();
      }
    } else if (len + 1 < (int)sizeof(line)) line[len++] = c;
  }
}

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = true;
  cfg.internal_mic = true;
  cfg.internal_spk = false;
  cfg.fallback_board = m5::board_t::board_M5StopWatch;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(255);
  canvas.setColorDepth(8);
  canvas.createSprite(466, 466);
  M5.Mic.begin();

  recording = (int16_t *)ps_malloc(MAX_SAMPLES * sizeof(int16_t));

  wifiPrefs.begin("day22", false); // shared Wi-Fi home
  notePrefs.begin("day28", false);
  notesLoad();
  notesSaved = noteTotal;
  const String ssid = wifiPrefs.getString("ssid", "");
  if (!ssid.length()) {
    // No Wi-Fi: become the setup page. The portal also collects this
    // day's service settings, then reboots provisioned.
    canvas.fillSprite(TFT_BLACK);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(0xFD20, TFT_BLACK);
    canvas.setTextSize(3);
    canvas.drawString("SETUP", 233, 140);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("join Wi-Fi network", 233, 210);
    canvas.drawString("\"esptember-setup\"", 233, 244);
    canvas.setTextColor(TFT_DARKGRAY, TFT_BLACK);
    canvas.drawString("the setup page opens itself", 233, 300);
    canvas.pushSprite(0, 0);
    static const PortalField fields[] = {
        {"dgkey", "Deepgram API key", true},
    };
    WifiPortal portal;
    portal.run(wifiPrefs, notePrefs, fields, 1); // reboots on save
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), wifiPrefs.getString("pass", "").c_str());
  render();
  Serial.println("D28_READY");
}

void loop() {
  M5.update();

  // PTT: the crown records for as long as it's held (or until full).
  if (M5.BtnA.isPressed() && state != State::Recording &&
      WiFi.status() == WL_CONNECTED) {
    state = State::Recording;
    recorded = 0;
    render();
  }
  if (state == State::Recording) {
    if (M5.BtnA.isPressed() && recorded + CHUNK <= MAX_SAMPLES) {
      if (M5.Mic.record(recording + recorded, CHUNK, SAMPLE_RATE)) {
        recorded += CHUNK;
        if (recorded % SAMPLE_RATE < CHUNK) render(); // once a second
      }
    } else {
      // Released (or full): the pipeline runs to completion, blocking —
      // a memo is a moment, and the moment can wait two seconds.
      Serial.printf("D28_RECORDED samples=%u\n", (unsigned)recorded);
      if (recorded < SAMPLE_RATE / 2) { // sub-half-second: ignore
        state = State::Idle;
        render();
      } else {
        state = State::Transcribing;
        render();
        String text;
        if (transcribe(text)) {
          strlcpy(lastNote, text.c_str(), sizeof(lastNote));
          Serial.printf("D28_TRANSCRIPT %s\n", lastNote);
          state = State::Saving;
          render();
          if (saveNoteLocal(text)) {
            notesSaved++;
            state = State::Done;
            Serial.println("D28_FILED");
          } else {
            state = State::Error;
            Serial.printf("D28_SAVE_FAILED %s\n", statusLine);
          }
        } else {
          state = State::Error;
          Serial.printf("D28_TRANSCRIBE_FAILED %s\n", statusLine);
        }
        render();
        reportStatus();
      }
    }
  }
  // B browses the local ring (newest first); it also dismisses results.
  if (M5.BtnB.wasClicked() && state != State::Recording) {
    if (state == State::Done || state == State::Error) {
      state = State::Idle;
      browseIndex = -1;
    } else {
      const int stored = noteTotal < NOTE_SLOTS ? noteTotal : NOTE_SLOTS;
      if (stored) {
        browseIndex = (browseIndex + 1) % stored;
        strlcpy(lastNote, readNote(browseIndex).c_str(), sizeof(lastNote));
        state = State::Done; // reuse the note display
        Serial.printf("D28_BROWSE %d %s\n", browseIndex, lastNote);
      }
    }
    render();
  }

  // Hold both pushers ~2 s: forget Wi-Fi, reopen the portal.
  static uint32_t chordSince = 0;
  if (M5.BtnA.isPressed() && M5.BtnB.isPressed()) {
    if (!chordSince) chordSince = millis();
    else if (millis() - chordSince > 2000) {
      wifiPrefs.clear();
      Serial.println("D28_WIFI_RESET");
      ESP.restart();
    }
  } else {
    chordSince = 0;
  }

  handleSerial();
  delay(2);
}
