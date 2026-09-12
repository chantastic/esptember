#include <M5Unified.h>
#include <Preferences.h>
#include <time.h>
#include "workouts.h"
#include "session.h"
#include "buttons.h"
#include "progress.h"
#include "feedback.h"
#include "ui.h"

// The RTC stores local civil time. Override for another timezone when building.
#ifndef C25K_TIMEZONE
#define C25K_TIMEZONE "PST8PDT,M3.2.0,M11.1.0"
#endif

using namespace c25k;
Preferences preferences;
Progress progress, diagnosticBackup;
Session session;
ButtonGrammar buttons;
Feedback feedback;
Ui ui;
Screen screen = Screen::Home;
uint8_t homePage = 0, settingsPage = 0, timeField = 0;
uint8_t editHour = 0, editMinute = 0;
m5::rtc_date_t editDate;
uint16_t historyPage = 0;
bool storageOk = true, storageWritable = true, prefsOpen = false;
bool dirty = true, diagnostic = false, displayReady = false;
uint32_t diagnosticOffset = 0, overlayStart = 0, lastDraw = 0;
uint32_t maxLoopUs = 0, previousLoopUs = 0, physicalInputs = 0;
int batteryPercent = -1;
char command[96]{};
size_t commandLength = 0;
bool commandOverflow = false;
uint16_t* capturePixels = nullptr;
size_t capturePosition = 0, captureLength = 0;
uint32_t captureStart = 0;

uint32_t appNow() { return millis() + diagnosticOffset; }

bool readClock(m5::rtc_datetime_t& dt) {
  return M5.Rtc.isEnabled() && M5.Rtc.getDateTime(&dt) &&
         dt.date.year >= 2025 && dt.date.year <= 2099 &&
         dt.date.month >= 1 && dt.date.month <= 12 &&
         dt.date.date >= 1 && dt.date.date <= 31 &&
         dt.time.hours >= 0 && dt.time.hours <= 23 &&
         dt.time.minutes >= 0 && dt.time.minutes <= 59 &&
         dt.time.seconds >= 0 && dt.time.seconds <= 59;
}

uint32_t rtcTimestamp() {
  m5::rtc_datetime_t dt;
  if (!readClock(dt)) return 0;
  struct tm t = dt.get_tm(); t.tm_isdst = -1;
  time_t value = mktime(&t);
  return value > 0 && uint64_t(value) <= UINT32_MAX ? uint32_t(value) : 0;
}

void formatDate(uint32_t stamp, char* out, size_t length, bool includeTime) {
  if (!stamp) { snprintf(out, length, "--"); return; }
  time_t value = stamp;
  struct tm t{}; localtime_r(&value, &t);
  strftime(out, length, includeTime ? "%Y-%m-%d  %H:%M" : "%Y-%m-%d", &t);
}

bool saveProgress() {
  progress.seal();
  if (diagnostic) return true;
  storageOk = storageWritable && prefsOpen &&
      preferences.putBytes("state", &progress, sizeof(progress)) == sizeof(progress);
  if (!storageOk && !capturePixels) Serial.println("C25K_STORAGE_ERROR");
  return storageOk;
}

void loadProgress() {
  progress.defaults();
  prefsOpen = preferences.begin("c25k", false);
  if (!prefsOpen) { storageOk = storageWritable = false; return; }
  const size_t length = preferences.getBytesLength("state");
  if (!length) return;
  Progress saved{};
  if (length == sizeof(saved) && preferences.getBytes("state", &saved, sizeof(saved)) == sizeof(saved)
      && saved.valid()) progress = saved;
  else {
    // Preserve unknown versions/corrupt bytes until the user explicitly resets.
    storageOk = storageWritable = false;
  }
}

void applyFeedbackSettings() {
  feedback.enable(!diagnostic && progress.sound_on, !diagnostic && progress.vib_on);
}

void home() {
  screen = Screen::Home; homePage = progress.cursor; dirty = true;
}

void completeWorkout() {
  LogEntry entry{session.workoutIndex, rtcTimestamp(), logSeconds(session.totalElapsedMs),
                 logSeconds(session.runElapsedMs), uint8_t(session.partial ? 1 : 0)};
  progress.append(entry); saveProgress();
  screen = Screen::Complete; dirty = true;
  if (!capturePixels)
    Serial.printf("C25K_COMPLETE workout=%u partial=%u total=%u run=%u saved=%u test=%u\n",
                  entry.workout, entry.flags, entry.total_sec, entry.run_sec, storageOk, diagnostic);
}

void drainSessionEvents() {
  SessionEvent event;
  while (session.popEvent(event)) {
    feedback.cue(event, millis()); dirty = true;
    if (event == SessionEvent::Complete) completeWorkout();
  }
}

void beginSetTime() {
  m5::rtc_datetime_t dt;
  if (readClock(dt)) {
    editDate = dt.date; editHour = dt.time.hours; editMinute = dt.time.minutes;
  } else {
    // The requested hour/minute flow has no date editor; use the visible build
    // date only when the user explicitly confirms an unset clock.
    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char month[4]{}; int day = 1, year = 2026;
    sscanf(__DATE__, "%3s %d %d", month, &day, &year);
    const char* found = strstr(months, month);
    editDate = m5::rtc_date_t(year, found ? ((found - months) / 3 + 1) : 1, day);
    editHour = 12; editMinute = 0;
  }
  timeField = 0; screen = Screen::SetTime;
}

void handleInput(Input input) {
  if (input == Input::None) return;
  const uint32_t now = appNow();
  session.update(now); drainSessionEvents();
  if (input != Input::Escape) feedback.tap(millis());
  dirty = true;
  switch (screen) {
    case Screen::Home:
      if (input == Input::Next) homePage = (homePage + 1) % 28;
      else if (input == Input::Prev) homePage = (homePage + 27) % 28;
      else if (input == Input::Escape) {
        settingsPage = 0; screen = Screen::Settings;
        batteryPercent = M5.Power.getBatteryLevel();
      } else if (input == Input::Enter) {
        if (homePage == 27) { historyPage = 0; screen = Screen::History; }
        else { session.start(homePage, now); screen = Screen::Workout; }
      }
      break;
    case Screen::Workout:
      if (input == Input::Next) session.next(now);
      else if (input == Input::Prev) session.prev(now);
      else if (input == Input::Enter) session.togglePause(now);
      else if (input == Input::Escape) {
        session.resetPrevChain(); screen = Screen::CancelConfirm; overlayStart = now;
      }
      break;
    case Screen::CancelConfirm:
      session.resetPrevChain();
      if (input == Input::Escape) { session.cancel(); feedback.stop(); home(); }
      else screen = Screen::Workout;
      break;
    case Screen::Complete:
      if (input == Input::Enter || input == Input::Escape) home();
      break;
    case Screen::History:
      if (input == Input::Escape) home();
      else if (progress.count && input == Input::Next) historyPage = (historyPage + 1) % progress.count;
      else if (progress.count && input == Input::Prev)
        historyPage = (historyPage + progress.count - 1) % progress.count;
      break;
    case Screen::Settings:
      if (input == Input::Escape) home();
      else if (input == Input::Next) settingsPage = (settingsPage + 1) % 5;
      else if (input == Input::Prev) settingsPage = (settingsPage + 4) % 5;
      else if (input == Input::Enter) {
        if (settingsPage == 0) { progress.sound_on = !progress.sound_on; saveProgress(); applyFeedbackSettings(); }
        else if (settingsPage == 1) { progress.vib_on = !progress.vib_on; saveProgress(); applyFeedbackSettings(); }
        else if (settingsPage == 2) beginSetTime();
        else if (settingsPage == 3) screen = Screen::ResetConfirm;
      }
      if (settingsPage == 4) batteryPercent = M5.Power.getBatteryLevel();
      break;
    case Screen::SetTime:
      if (input == Input::Escape) screen = Screen::Settings;
      else if (input == Input::Enter) {
        if (timeField < 2) ++timeField;
        else {
          if (!diagnostic) {
            m5::rtc_datetime_t dt(editDate, m5::rtc_time_t(editHour, editMinute, 0));
            M5.Rtc.setDateTime(dt);
          }
          screen = Screen::Settings;
        }
      } else if (timeField < 2) {
        const int step = input == Input::Next ? 1 : -1;
        if (timeField == 0) editHour = (editHour + 24 + step) % 24;
        else editMinute = (editMinute + 60 + step) % 60;
      }
      break;
    case Screen::ResetConfirm:
      if (input == Input::Escape) screen = Screen::Settings;
      else if (input == Input::Enter) {
        progress.resetProgress();
        if (!diagnostic) storageWritable = prefsOpen;
        saveProgress(); screen = Screen::Settings;
      }
      break;
  }
  drainSessionEvents();
}

void render() {
  if (!displayReady) return;
  UiView view{};
  view.screen = screen; view.homePage = homePage; view.cursor = progress.cursor;
  view.settingsPage = settingsPage; view.timeField = timeField;
  view.hour = editHour; view.minute = editMinute;
  if (screen == Screen::Settings && settingsPage == 2) {
    m5::rtc_datetime_t dt;
    if (readClock(dt)) { view.hour = dt.time.hours; view.minute = dt.time.minutes; }
  }
  view.programComplete = progress.program_complete;
  view.soundOn = progress.sound_on; view.vibOn = progress.vib_on;
  view.storageOk = storageOk; view.battery = batteryPercent;
  view.historyCount = progress.count; view.historyPage = historyPage;
  view.workoutIndex = session.workoutIndex; view.segmentIndex = session.segmentIndex;
  view.segmentCount = workoutAt(session.workoutIndex).count;
  view.remainingSec = (session.remainingMs() + 999) / 1000;
  view.elapsedSec = session.totalElapsedMs / 1000; view.runSec = session.runElapsedMs / 1000;
  view.paused = session.paused; view.partial = session.partial;
  view.plannedSec = totalSeconds(workoutAt(session.workoutIndex));
  view.segmentProgress = float(session.segmentElapsedMs) / (session.currentSegment().seconds * 1000.0f);
  view.sessionProgress = float(session.progressMs()) / (view.plannedSec * 1000.0f);
  char date[32]{}, lastDate[24]{};
  view.rtcValid = rtcTimestamp() != 0;
  if (screen == Screen::Home && homePage < 27) {
    view.workoutIndex = homePage; view.plannedSec = totalSeconds(workoutAt(homePage));
    bool found = false;
    for (uint16_t i = 0; i < progress.count; ++i) {
      const auto* entry = progress.newest(i);
      if (entry->workout != homePage) continue;
      if (entry->flags & 1) ++view.partialCount;
      else ++view.completionCount;
      if (!found) { formatDate(entry->timestamp, lastDate, sizeof(lastDate), false); found = true; }
    }
  } else if (screen == Screen::History && progress.count) {
    const auto* entry = progress.newest(historyPage);
    view.workoutIndex = entry->workout; view.partial = entry->flags & 1;
    view.elapsedSec = entry->total_sec; view.runSec = entry->run_sec;
    formatDate(entry->timestamp, date, sizeof(date), true);
  } else if (screen == Screen::SetTime) {
    snprintf(date, sizeof(date), "%04d-%02d-%02d", editDate.year, editDate.month, editDate.date);
  } else formatDate(rtcTimestamp(), date, sizeof(date), true);
  view.dateText = date; view.lastDateText = lastDate;
  ui.draw(view); ui.push(); dirty = false; lastDraw = millis();
}

const char* screenName() {
  switch (screen) {
    case Screen::Home: return "home"; case Screen::Workout: return "workout";
    case Screen::CancelConfirm: return "cancel"; case Screen::Complete: return "complete";
    case Screen::History: return "history"; case Screen::Settings: return "settings";
    case Screen::SetTime: return "set_time"; case Screen::ResetConfirm: return "reset";
  }
  return "unknown";
}

void reportStatus() {
  Serial.printf("C25K_STATUS screen=%s page=%u cursor=%u workout=%u segment=%u remaining=%lu total_ms=%llu run_ms=%llu paused=%u partial=%u logs=%u sound=%u vibration=%u storage=%u rtc=%lu test=%u inputs=%lu loop_max_us=%lu psram=%u width=%d height=%d speaker=%u\n",
    screenName(), homePage, progress.cursor, session.workoutIndex, session.segmentIndex,
    (unsigned long)session.remainingMs(), (unsigned long long)session.totalElapsedMs,
    (unsigned long long)session.runElapsedMs, session.paused, session.partial, progress.count,
    progress.sound_on, progress.vib_on, storageOk, (unsigned long)rtcTimestamp(), diagnostic,
    (unsigned long)physicalInputs, (unsigned long)maxLoopUs, ESP.getPsramSize(),
    M5.Display.width(), M5.Display.height(), feedback.speakerReady);
}

void startCapture() {
  if (!ui.buffered() || capturePixels) { Serial.println("C25K_CAPTURE_REJECTED"); return; }
  render();
  captureLength = size_t(M5.Display.width()) * M5.Display.height() * 2;
  capturePixels = static_cast<uint16_t*>(ps_malloc(captureLength));
  if (!capturePixels) { Serial.println("C25K_CAPTURE_REJECTED"); return; }
  // Explicit pixel type avoids M5GFX's uint16_t overload, which swaps bytes.
  ui.canvas().readRect(0, 0, M5.Display.width(), M5.Display.height(),
                       reinterpret_cast<lgfx::rgb565_t*>(capturePixels));
  capturePosition = 0; captureStart = millis();
  Serial.printf("C25K_CAPTURE %d %d RGB565LE\n", M5.Display.width(), M5.Display.height());
}

void updateCapture() {
  if (!capturePixels) return;
  if (uint32_t(millis() - captureStart) > 10000) {
    free(capturePixels); capturePixels = nullptr; Serial.println("\nC25K_CAPTURE_ABORTED"); return;
  }
  int available = Serial.availableForWrite();
  if (available <= 0) return;
  if (capturePosition == captureLength) {
    // The binary tail can fill the TX buffer. Queue the terminator on a later
    // loop with room, rather than dropping it with our zero USB write timeout.
    constexpr char ending[] = "\nC25K_CAPTURE_END\n";
    if (available < int(sizeof(ending) - 1)) return;
    Serial.write(reinterpret_cast<const uint8_t*>(ending), sizeof(ending) - 1);
    free(capturePixels); capturePixels = nullptr; return;
  }
  size_t amount = captureLength - capturePosition;
  if (amount > 512) amount = 512;
  if (amount > size_t(available)) amount = available;
  capturePosition += Serial.write(reinterpret_cast<uint8_t*>(capturePixels) + capturePosition, amount);
}

void runCommand(const char* value) {
  if (!strcmp(value, "status")) { reportStatus(); return; }
  if (!strcmp(value, "capture")) { startCapture(); return; }
  if (!strcmp(value, "reboot")) { feedback.stop(); ESP.restart(); return; }
  if (!strcmp(value, "test begin") && !diagnostic && !session.active) {
    diagnosticBackup = progress; diagnostic = true; diagnosticOffset = 0;
    progress.defaults(); feedback.stop(); applyFeedbackSettings(); home();
  } else if (!strcmp(value, "test end") && diagnostic) {
    session.cancel(); diagnostic = false; diagnosticOffset = 0;
    progress = diagnosticBackup; feedback.stop(); applyFeedbackSettings(); home();
  } else if (!strncmp(value, "advance ", 8) && diagnostic && session.active) {
    char* end = nullptr; unsigned long amount = strtoul(value + 8, &end, 10);
    if (!end || *end || amount > 3600000) { Serial.println("C25K_REJECTED"); return; }
    diagnosticOffset += uint32_t(amount); session.update(appNow()); drainSessionEvents(); dirty = true;
  } else if (!strncmp(value, "time ", 5) && !diagnostic && !session.active) {
    char* end = nullptr; unsigned long stamp = strtoul(value + 5, &end, 10);
    if (!end || *end || stamp < 1735689600UL || stamp > 4102444799UL) {
      Serial.println("C25K_REJECTED"); return;
    }
    time_t timestamp = stamp; struct tm t{}; localtime_r(&timestamp, &t);
    M5.Rtc.setDateTime(&t); dirty = true;
  } else if (!strcmp(value, "next")) handleInput(Input::Next);
  else if (!strcmp(value, "prev")) handleInput(Input::Prev);
  else if (!strcmp(value, "enter")) handleInput(Input::Enter);
  else if (!strcmp(value, "escape")) handleInput(Input::Escape);
  else if (!strcmp(value, "metrics reset")) maxLoopUs = 0;
  else { Serial.println("C25K_REJECTED"); return; }
  Serial.println("C25K_OK"); reportStatus();
}

void readCommands() {
  // Bounded USB work; a stalled capture/host never owns the timer loop.
  if (capturePixels) return;
  for (int i = 0; i < 96 && Serial.available(); ++i) {
    const char ch = Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      command[commandLength] = 0;
      if (commandOverflow) Serial.println("C25K_REJECTED");
      else if (commandLength) runCommand(command);
      commandLength = 0; commandOverflow = false; return;
    }
    if (commandLength + 1 < sizeof(command)) command[commandLength++] = ch;
    else commandOverflow = true;
  }
}

void setup() {
  Serial.setTxBufferSize(4096); Serial.begin(115200); Serial.setTxTimeoutMs(0);
  auto cfg = M5.config();
  cfg.internal_imu = false; cfg.internal_rtc = true;
  cfg.internal_mic = false; cfg.internal_spk = true;
  cfg.fallback_board = m5::board_t::board_M5StopWatch;
  M5.begin(cfg);
  M5.Display.setRotation(0); M5.Display.setBrightness(255);
  setenv("TZ", C25K_TIMEZONE, 1); tzset();
  loadProgress(); feedback.begin(); applyFeedbackSettings();
  ui.begin(); displayReady = true;  // Ui has a direct-display allocation fallback.
  home(); render();
  Serial.println("C25K_READY version=1.0.0"); reportStatus();
}

void loop() {
  const uint32_t loopUs = micros();
  if (previousLoopUs) {
    const uint32_t gap = loopUs - previousLoopUs;
    if (gap > maxLoopUs) maxLoopUs = gap;
  }
  previousLoopUs = loopUs;
  M5.update();
  const uint32_t now = appNow();
  session.update(now); drainSessionEvents();
  Input input = buttons.update(millis(), M5.BtnA.isPressed(), M5.BtnB.isPressed(),
                              M5.BtnA.wasClicked(), M5.BtnB.wasClicked());
  if (input != Input::None) { ++physicalInputs; handleInput(input); }
  if (screen == Screen::CancelConfirm && uint32_t(appNow() - overlayStart) >= 5000) {
    screen = Screen::Workout; dirty = true;
  }
  feedback.update(millis());
  readCommands(); updateCapture();
  if (dirty || ((screen == Screen::Workout || screen == Screen::CancelConfirm) &&
                !session.paused && uint32_t(millis() - lastDraw) >= 100)) render();
  yield();
}
