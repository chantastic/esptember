#pragma once

#include <M5Unified.h>
#include <math.h>
#include "workouts.h"
#include "workout_summary.h"

namespace c25k {

enum class Screen : uint8_t {
  Home, Workout, CancelConfirm, Complete, History, Settings, SetTime, ResetConfirm, ResetResult
};

// A frame is a snapshot: drawing never changes the timer or persistent state.
struct UiView {
  Screen screen = Screen::Home;
  uint8_t homePage = 0, cursor = 0, settingsPage = 0, timeField = 0;
  uint8_t hour = 0, minute = 0;
  bool programComplete = false, soundOn = true, vibOn = true;
  bool partial = false, paused = false, rtcValid = true, storageOk = true;
  bool resetSucceeded = false;
  uint8_t workoutIndex = 0, segmentIndex = 0, segmentCount = 0;
  uint32_t remainingSec = 0, elapsedSec = 0, runSec = 0, plannedSec = 0;
  uint32_t completedWorkouts = 0, completedSegments = 0;
  float segmentProgress = 0, sessionProgress = 0;
  uint16_t historyCount = 0, historyPage = 0, completionCount = 0, partialCount = 0;
  int battery = -1;
  const char* dateText = "";
  const char* lastDateText = "";
};

class Ui {
 public:
  bool begin() {
    _cx = M5.Display.width() / 2;
    _cy = M5.Display.height() / 2;
    _canvas.setColorDepth(16);
    _canvas.setPsram(true);
    _ready = _canvas.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;
    // A failed PSRAM allocation still leaves an operable, directly drawn UI.
    surface().setTextWrap(false, false);
    surface().fillScreen(BLACK);
    return _ready;
  }

  void draw(const UiView& v) {
    surface().fillScreen(BLACK);
    switch (v.screen) {
      case Screen::Home: home(v); break;
      case Screen::Workout: workout(v); break;
      case Screen::CancelConfirm: workout(v); cancelConfirm(); break;
      case Screen::Complete: complete(v); break;
      case Screen::History: history(v); break;
      case Screen::Settings: settings(v); break;
      case Screen::SetTime: setTime(v); break;
      case Screen::ResetConfirm: resetConfirm(v); break;
      case Screen::ResetResult: resetResult(v); break;
    }
  }

  void push() { if (_ready) _canvas.pushSprite(0, 0); }
  M5Canvas& canvas() { return _canvas; }
  bool buffered() const { return _ready; }

 private:
  static constexpr uint32_t BLACK = 0x000000;
  static constexpr uint32_t WHITE = 0xF4F4F2;
  static constexpr uint32_t MUTED = 0x9B9DA2;
  static constexpr uint32_t DIM = 0x53565B;
  static constexpr uint32_t PAUSED = 0x85888D;
  static constexpr uint32_t TRACK = 0x24272B;
  static constexpr uint32_t RUN = 0xFF765E;
  static constexpr uint32_t WALK = 0x52D9CB;
  M5Canvas _canvas{&M5.Display};
  bool _ready = false;
  int _cx = 234, _cy = 234;

  lgfx::LGFXBase& surface() {
    if (_ready) return _canvas;
    return M5.Display;
  }

  void text(const char* str, int y, const lgfx::IFont* font,
            uint32_t color = WHITE, int dx = 0) {
    auto& g = surface();
    g.setFont(font);
    g.setTextSize(1);
    g.setTextColor(color, BLACK);
    g.setTextDatum(lgfx::textdatum_t::middle_center);
    g.drawString(str ? str : "", _cx + dx, y);
  }

  static void duration(char* dst, size_t n, uint32_t seconds) {
    snprintf(dst, n, "%lu:%02lu", static_cast<unsigned long>(seconds / 60),
             static_cast<unsigned long>(seconds % 60));
  }

  static void workoutLabel(char* dst, size_t n, uint8_t index) {
    snprintf(dst, n, "W%u D%u", unsigned(index / 3 + 1), unsigned(index % 3 + 1));
  }

  static uint32_t segmentColor(SegmentType type) {
    return type == SegmentType::Run ? RUN : WALK;
  }

  static const char* segmentLabel(SegmentType type) {
    switch (type) {
      case SegmentType::Warmup: return "WARM UP";
      case SegmentType::Run: return "RUN";
      case SegmentType::Walk: return "WALK";
      case SegmentType::Cooldown: return "COOL DOWN";
    }
    return "";
  }

  void smallHint(const char* hint) { text(hint, 364, &fonts::DejaVu18, MUTED); }

  void check(int x, int y, uint32_t color, int scale = 1) {
    auto& g = surface();
    for (int i = 0; i < 3 * scale; ++i) {
      g.drawLine(x - 9 * scale, y + i, x - 2 * scale, y + 7 * scale + i, color);
      g.drawLine(x - 2 * scale, y + 7 * scale + i, x + 12 * scale, y - 8 * scale + i, color);
    }
  }

  void arcDots(unsigned count, int current, uint32_t completed = 0,
               const Workout* plan = nullptr, int viewed = -1) {
    if (!count) return;
    auto& g = surface();
    // Keep large histories readable with a moving window around the current page.
    unsigned visible = count > 28 ? 21 : count;
    unsigned first = 0;
    if (count > visible && current > int(visible / 2)) {
      first = unsigned(current) - visible / 2;
      if (first + visible > count) first = count - visible;
    }
    const float span = visible > 18 ? 120.0f : (visible > 8 ? 100.0f : 58.0f);
    for (unsigned j = 0; j < visible; ++j) {
      unsigned i = first + j;
      float angle = visible > 1 ? (-span / 2 + span * j / (visible - 1)) : 0;
      float radians = angle * 0.01745329252f;
      int x = _cx + lroundf(sinf(radians) * 187);
      int y = _cy + lroundf(cosf(radians) * 187);
      const uint32_t color = plan ? segmentColor(plan->segments[i].type) : WHITE;
      const int r = count > 20 ? 4 : 5;
      const bool done = i < 32 && (completed & (uint32_t(1) << i));
      if (int(i) == current) {
        // Current takes precedence while replaying an already finished item.
        g.fillCircle(x, y, r + 2, color);
        g.fillCircle(x, y, r - 1, BLACK);
      } else if (done) {
        g.fillCircle(x, y, r, color);
      } else {
        g.drawCircle(x, y, r, plan ? color : DIM);
      }
      if (int(i) == viewed) {
        // Browsing a Home page must not make that workout look completed or
        // change which workout is next. An outer tick clears the footer text.
        for (int offset = -1; offset <= 1; ++offset) {
          const int dx = lroundf(cosf(radians) * offset);
          const int dy = -lroundf(sinf(radians) * offset);
          g.drawLine(_cx + lroundf(sinf(radians) * 198) + dx,
                     _cy + lroundf(cosf(radians) * 198) + dy,
                     _cx + lroundf(sinf(radians) * 203) + dx,
                     _cy + lroundf(cosf(radians) * 203) + dy, WHITE);
        }
      }
    }
  }

  void ring(int radius, int thickness, float progress, uint32_t color) {
    auto& g = surface();
    g.fillArc(_cx, _cy, radius, radius - thickness, 0, 360, TRACK);
    if (progress <= 0) return;
    if (progress > 1) progress = 1;
    g.fillArc(_cx, _cy, radius, radius - thickness, -90, -90 + progress * 360, color);
  }

  // Vector numerals have a true 122 px glyph height, independent of font metrics.
  // Their chamfered strokes remain crisp on the AMOLED at running distance.
  void digit(int value, int x, int y, int w, int h, uint32_t color) {
    static constexpr uint8_t masks[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66,
                                        0x6D, 0x7D, 0x07, 0x7F, 0x6F};
    uint8_t mask = masks[value % 10];
    const int t = 13, mid = h / 2;
    auto& g = surface();
    auto horizontal = [&](int yy) {
      g.fillRect(x + t, yy, w - 2 * t, t, color);
      g.fillTriangle(x + t, yy, x + t, yy + t - 1, x + t / 2, yy + t / 2, color);
      g.fillTriangle(x + w - t - 1, yy, x + w - t - 1, yy + t - 1,
                     x + w - t / 2 - 1, yy + t / 2, color);
    };
    auto vertical = [&](int xx, int yy) {
      const int length = mid - t;
      g.fillRect(xx, yy + t / 2, t, length - t, color);
      g.fillTriangle(xx, yy + t / 2, xx + t - 1, yy + t / 2,
                     xx + t / 2, yy, color);
      g.fillTriangle(xx, yy + length - t / 2 - 1,
                     xx + t - 1, yy + length - t / 2 - 1,
                     xx + t / 2, yy + length - 1, color);
    };
    if (mask & 0x01) horizontal(y);
    if (mask & 0x02) vertical(x + w - t, y + t);
    if (mask & 0x04) vertical(x + w - t, y + mid + t / 2);
    if (mask & 0x08) horizontal(y + h - t);
    if (mask & 0x10) vertical(x, y + mid + t / 2);
    if (mask & 0x20) vertical(x, y + t);
    if (mask & 0x40) horizontal(y + mid - t / 2);
  }

  void countdown(uint32_t seconds, uint32_t color) {
    char value[12];
    duration(value, sizeof(value), seconds);
    const int len = strlen(value);
    const int digitWidth = len > 4 ? 64 : 75;
    const int colonWidth = 18, gap = 9, height = 122, top = 159;
    const int width = (len - 1) * digitWidth + colonWidth + (len - 1) * gap;
    int x = _cx - width / 2;
    for (int i = 0; i < len; ++i) {
      if (value[i] == ':') {
        surface().fillCircle(x + colonWidth / 2, top + 40, 6, color);
        surface().fillCircle(x + colonWidth / 2, top + 82, 6, color);
        x += colonWidth + gap;
      } else {
        digit(value[i] - '0', x, top, digitWidth, height, color);
        x += digitWidth + gap;
      }
    }
  }

  void activitySummary(const Workout& plan) {
    const WorkoutSummary summary = summarizeWorkout(plan);
    char label[48], warmup[12], cooldown[12];
    if (summary.repeats > 1)
      snprintf(label, sizeof(label), "%u ROUNDS", unsigned(summary.repeats));
    else if (summary.patternCount == 1)
      snprintf(label, sizeof(label), "CONTINUOUS RUN");
    else
      snprintf(label, sizeof(label), "%u INTERVALS", unsigned(summary.coreCount));
    text(label, 232, &fonts::DejaVu12, MUTED);

    // Two large activities for a repeated pair; longer sequences flow across
    // at most three rows. Every token comes from the same table as the timer.
    const unsigned perRow = summary.patternCount <= 4 ? 2 : 3;
    const unsigned rows = (summary.patternCount + perRow - 1) / perRow;
    const lgfx::IFont* font = summary.patternCount <= 2 ? &fonts::DejaVu24 : &fonts::DejaVu18;
    auto& g = surface();
    for (unsigned row = 0; row < rows; ++row) {
      char tokens[3][24]{};
      int widths[3]{}, width = 0;
      unsigned first = row * perRow;
      unsigned count = summary.patternCount - first;
      if (count > perRow) count = perRow;
      g.setFont(font); g.setTextSize(1);
      for (unsigned i = 0; i < count; ++i) {
        const Segment& segment = plan.segments[summary.firstCore + first + i];
        char time[12]; duration(time, sizeof(time), segment.seconds);
        snprintf(tokens[i], sizeof(tokens[i]), "%s %s",
                 segment.type == SegmentType::Run ? "RUN" : "WALK", time);
        widths[i] = g.textWidth(tokens[i]); width += widths[i];
      }
      constexpr int gap = 18;
      width += (count - 1) * gap;
      int x = _cx - width / 2;
      int y = rows == 1 ? 276 : rows == 2 ? 261 + row * 30 : 251 + row * 26;
      for (unsigned i = 0; i < count; ++i) {
        const Segment& segment = plan.segments[summary.firstCore + first + i];
        text(tokens[i], y, font, segmentColor(segment.type), x + widths[i] / 2 - _cx);
        x += widths[i];
        if (i + 1 < count) {
          g.drawLine(x + 6, y - 3, x + 10, y, DIM);
          g.drawLine(x + 10, y, x + 6, y + 3, DIM);
        }
        x += gap;
      }
    }
    duration(warmup, sizeof(warmup), summary.warmupSeconds);
    duration(cooldown, sizeof(cooldown), summary.cooldownSeconds);
    snprintf(label, sizeof(label), "%s WARM UP + %s COOL DOWN", warmup, cooldown);
    text(label, 327, &fonts::DejaVu12, MUTED);
  }

  void home(const UiView& v) {
    text(!v.storageOk ? "STORAGE UNAVAILABLE" : v.programComplete ? "PROGRAM COMPLETE" : "COUCH TO 5K",
         v.homePage >= 27 ? 92 : 82, &fonts::DejaVu12, MUTED);
    if (v.homePage >= 27) {
      historyIcon(174);
      text("History", 250, &fonts::DejaVu40);
      char count[40];
      snprintf(count, sizeof(count), "%u saved %s", unsigned(v.historyCount),
               v.historyCount == 1 ? "session" : "sessions");
      text(count, 300, &fonts::DejaVu18, MUTED);
      smallHint("BOTH TO OPEN");
    } else {
      char label[24], time[16], count[48];
      workoutLabel(label, sizeof(label), v.homePage);
      text(label, 132, &fonts::DejaVu56);
      if (!v.programComplete && v.homePage == v.cursor) {
        surface().fillTriangle(_cx - 5, 175, _cx + 5, 175, _cx, 168, WHITE);
      }
      duration(time, sizeof(time), totalSeconds(workoutAt(v.homePage)));
      snprintf(label, sizeof(label), "%s total", time);
      text(label, 199, &fonts::DejaVu24);
      activitySummary(workoutAt(v.homePage));
      if (v.completionCount || v.partialCount) {
        if (v.completionCount && v.partialCount) {
          snprintf(count, sizeof(count), "%u complete   ~%u partial",
                   unsigned(v.completionCount), unsigned(v.partialCount));
        } else if (v.partialCount) {
          snprintf(count, sizeof(count), "~%u partial %s", unsigned(v.partialCount),
                   v.partialCount == 1 ? "session" : "sessions");
        } else {
          snprintf(count, sizeof(count), "%u %s", unsigned(v.completionCount),
                   v.completionCount == 1 ? "completion" : "completions");
        }
        text(count, 349, &fonts::DejaVu12, MUTED);
        if (v.lastDateText && *v.lastDateText)
          text(v.lastDateText, 367, &fonts::DejaVu12, DIM);
      }
      text("BOTH TO START", 389, &fonts::DejaVu18, MUTED);
    }
    arcDots(28, v.programComplete ? -1 : v.cursor, v.completedWorkouts, nullptr, v.homePage);
  }

  void workout(const UiView& v) {
    const Workout& plan = workoutAt(v.workoutIndex);
    const unsigned index = v.segmentIndex < plan.count ? v.segmentIndex : plan.count - 1;
    SegmentType type = plan.segments[index].type;
    uint32_t accent = segmentColor(type);
    ring(225, 3, v.sessionProgress, WHITE);
    ring(213, 9, v.segmentProgress, v.paused ? DIM : accent);
    text(v.paused ? "PAUSED" : segmentLabel(type), 111, &fonts::DejaVu40,
         v.paused ? MUTED : accent);
    countdown(v.remainingSec, v.paused ? PAUSED : WHITE);
    char line[48], time[16];
    snprintf(line, sizeof(line), "Seg %u/%u", unsigned(index + 1), unsigned(plan.count));
    text(line, 320, &fonts::DejaVu18, MUTED);
    duration(time, sizeof(time), v.elapsedSec);
    snprintf(line, sizeof(line), "%s elapsed", time);
    text(line, 350, &fonts::DejaVu18, MUTED);
    arcDots(plan.count, index, v.completedSegments, &plan);
  }

  void cancelConfirm() {
    auto& g = surface();
    g.fillRoundRect(_cx - 163, 137, 326, 194, 22, BLACK);
    g.drawRoundRect(_cx - 163, 137, 326, 194, 22, DIM);
    text("Cancel workout?", 184, &fonts::DejaVu24);
    text("Hold both again", 235, &fonts::DejaVu18);
    text("to confirm", 262, &fonts::DejaVu18);
    text("Any press to keep going", 304, &fonts::DejaVu12, MUTED);
  }

  void metric(const char* label, uint32_t seconds, int y, int dx) {
    char time[16];
    duration(time, sizeof(time), seconds);
    auto& g = surface();
    g.setFont(&fonts::DejaVu40);
    g.setTextSize(1);
    int width = g.textWidth(time);
    g.setTextSize(width > 148 ? 148.0f / width : 1.0f);
    g.setTextColor(WHITE, BLACK);
    g.setTextDatum(lgfx::textdatum_t::middle_center);
    g.drawString(time, _cx + dx, y);
    text(label, y + 32, &fonts::DejaVu12, MUTED, dx);
  }

  void complete(const UiView& v) {
    check(_cx, 96, WHITE, 2);
    text(v.partial ? "DONE ~" : "DONE", 162, &fonts::DejaVu56);
    char label[16];
    workoutLabel(label, sizeof(label), v.workoutIndex);
    text(label, 225, &fonts::DejaVu24, MUTED);
    metric("TOTAL TIME", v.elapsedSec, 284, -81);
    metric("RUN TIME", v.runSec, 284, 81);
    smallHint(v.storageOk ? "BOTH TO CONTINUE" : "LOG COULD NOT BE SAVED");
  }

  void historyIcon(int y) {
    auto& g = surface();
    g.drawCircle(_cx, y, 32, WHITE);
    g.drawCircle(_cx, y, 31, WHITE);
    g.fillRoundRect(_cx - 2, y - 20, 4, 22, 2, WHITE);
    for (int i = -1; i <= 1; ++i)
      g.drawLine(_cx, y + i, _cx + 14, y + 9 + i, WHITE);
  }

  void history(const UiView& v) {
    text("HISTORY", 89, &fonts::DejaVu18, MUTED);
    if (!v.historyCount) {
      historyIcon(174);
      text("No runs yet", 253, &fonts::DejaVu24);
      text("Your first one starts here.", 295, &fonts::DejaVu18, MUTED);
      smallHint("HOLD BOTH TO GO BACK");
      return;
    }
    char label[24];
    workoutLabel(label, sizeof(label), v.workoutIndex);
    text(label, 151, &fonts::DejaVu40);
    text(v.dateText && *v.dateText ? v.dateText : "--", 205, &fonts::DejaVu18, MUTED);
    if (v.partial) {
      surface().drawRoundRect(_cx - 50, 230, 100, 25, 12, DIM);
      text("PARTIAL", 243, &fonts::DejaVu12, MUTED);
    }
    metric("TOTAL TIME", v.elapsedSec, 285, -81);
    metric("RUN TIME", v.runSec, 285, 81);
    snprintf(label, sizeof(label), "%u of %u", unsigned(v.historyPage + 1), unsigned(v.historyCount));
    text(label, 362, &fonts::DejaVu18, MUTED);
    arcDots(v.historyCount, v.historyPage);
  }

  void toggle(bool on, int y) {
    auto& g = surface();
    g.fillRoundRect(_cx - 52, y - 25, 104, 50, 25, on ? WHITE : TRACK);
    g.fillCircle(_cx + (on ? 27 : -27), y, 18, on ? BLACK : MUTED);
  }

  void settings(const UiView& v) {
    text("SETTINGS", 94, &fonts::DejaVu18, MUTED);
    switch (v.settingsPage) {
      case 0:
      case 1: {
        bool on = v.settingsPage == 0 ? v.soundOn : v.vibOn;
        toggle(on, 177);
        text(v.settingsPage == 0 ? "Sound" : "Vibration", 247, &fonts::DejaVu40);
        text(on ? "On" : "Off", 303, &fonts::DejaVu24, MUTED);
        smallHint("BOTH TO TOGGLE");
        break;
      }
      case 2: {
        historyIcon(175);
        text("Set Time", 246, &fonts::DejaVu40);
        char value[16];
        snprintf(value, sizeof(value), "%02u:%02u", unsigned(v.hour), unsigned(v.minute));
        text(v.rtcValid ? value : "Clock not set", 302, &fonts::DejaVu24, MUTED);
        smallHint("BOTH TO EDIT");
        break;
      }
      case 3:
        text("Reset", 187, &fonts::DejaVu40);
        text("Progress", 241, &fonts::DejaVu40);
        text("Start the program over", 302, &fonts::DejaVu18, MUTED);
        smallHint("BOTH TO OPEN");
        break;
      default: {
        text("C25K", 176, &fonts::DejaVu56);
        text("Version 1.0", 239, &fonts::DejaVu18, MUTED);
        text(__DATE__, 269, &fonts::DejaVu18, MUTED);
        char label[32];
        if (v.battery < 0) snprintf(label, sizeof(label), "Battery --");
        else snprintf(label, sizeof(label), "Battery %d%%", v.battery);
        text(label, 316, &fonts::DejaVu18, MUTED);
        smallHint("HOLD BOTH TO GO HOME");
        break;
      }
    }
    arcDots(5, v.settingsPage);
  }

  void setTime(const UiView& v) {
    text("SET TIME", 94, &fonts::DejaVu18, MUTED);
    text(v.timeField == 0 ? "Hour" : v.timeField == 1 ? "Minute" : "Save this time?",
         152, &fonts::DejaVu24);
    char hours[4], minutes[4];
    snprintf(hours, sizeof(hours), "%02u", unsigned(v.hour));
    snprintf(minutes, sizeof(minutes), "%02u", unsigned(v.minute));
    text(hours, 232, &fonts::DejaVu72, v.timeField == 1 ? DIM : WHITE, -76);
    text(":", 228, &fonts::DejaVu56, MUTED);
    text(minutes, 232, &fonts::DejaVu72, v.timeField == 0 ? DIM : WHITE, 76);
    if (v.timeField < 2) {
      int x = _cx + (v.timeField == 0 ? -76 : 76);
      surface().fillRoundRect(x - 43, 278, 86, 3, 1, WHITE);
      text("YELLOW -     BLUE +", 313, &fonts::DejaVu18, MUTED);
    } else if (v.dateText && *v.dateText) {
      text(v.dateText, 313, &fonts::DejaVu18, MUTED);
    }
    smallHint(v.timeField < 2 ? "BOTH FOR NEXT" : "BOTH TO SAVE");
    arcDots(3, v.timeField);
  }

  void resetConfirm(const UiView& v) {
    text("RESET PROGRESS", 102, &fonts::DejaVu18, MUTED);
    text("Start over?", 176, &fonts::DejaVu40);
    char label[48];
    snprintf(label, sizeof(label), "Clear %u saved %s", unsigned(v.historyCount),
             v.historyCount == 1 ? "session" : "sessions");
    text(label, 241, &fonts::DejaVu18);
    text("Return to W1 D1", 270, &fonts::DejaVu18);
    text("Hold both to cancel", 310, &fonts::DejaVu18, MUTED);
    text("PRESS BOTH + RELEASE", 354, &fonts::DejaVu18);
    text("TO RESET", 380, &fonts::DejaVu12, MUTED);
  }

  void resetResult(const UiView& v) {
    text("RESET PROGRESS", 94, &fonts::DejaVu18, MUTED);
    if (v.resetSucceeded) {
      check(_cx, 157, WALK, 2);
      text("History cleared", 230, &fonts::DejaVu24);
      text("W1 D1 is next", 276, &fonts::DejaVu24);
      text("Sound and vibration kept", 318, &fonts::DejaVu12, MUTED);
      smallHint("BOTH TO GO HOME");
    } else {
      text("Reset not confirmed", 181, &fonts::DejaVu24);
      text("Could not verify the save.", 239, &fonts::DejaVu18);
      text("Please try again.", 275, &fonts::DejaVu24);
      text("Hold both to go back", 321, &fonts::DejaVu18, MUTED);
      smallHint("BOTH TO RETRY");
    }
  }
};

}  // namespace c25k
