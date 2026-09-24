#include "lvgl_basics_core.h"

#include <cmath>
#include <cstdint>
#include <iostream>

namespace {

int assertions = 0;
int failures = 0;

#define CHECK(condition) do { \
  ++assertions; \
  if (!(condition)) { \
    ++failures; \
    std::cerr << __FILE__ << ':' << __LINE__ << ": CHECK failed: " #condition "\n"; \
  } \
} while (false)

void checkState(const lvglbasics::State& state, lvglbasics::Screen screen,
                lvglbasics::Focus focus, bool editing, int count,
                int brightness, bool orange) {
  CHECK(state.screen == screen);
  CHECK(state.focus == focus);
  CHECK(state.editingBrightness == editing);
  CHECK(state.pressCount == count);
  CHECK(state.brightness == brightness);
  CHECK(state.orangeMode == orange);
  CHECK(lvglbasics::valid(state));
}

void checkConstantsAndDefaults() {
  using namespace lvglbasics;
  CHECK(kDisplayWidth == 468);
  CHECK(kDisplayHeight == 466);
  CHECK(kPanelOffsetX == 6);
  CHECK(kPanelOffsetY == 0);
  CHECK(kMinimumBrightness == 10);
  CHECK(kMaximumBrightness == 100);
  CHECK(kBrightnessStep == 10);
  CHECK(kFocusInset == 3);
  CHECK(kControlCount == 4);
  checkState(initialState(), Screen::Controls, Focus::Count, false, 0, 100, false);
}

void checkButtons() {
  using namespace lvglbasics;
  State state = initialState();
  state = reduce(state, Input::Prev);
  checkState(state, Screen::Controls, Focus::About, false, 0, 100, false);
  state = reduce(state, Input::Next);
  checkState(state, Screen::Controls, Focus::Count, false, 0, 100, false);
  for (int i = 0; i < 5; ++i) state = reduce(state, Input::Next);
  checkState(state, Screen::Controls, Focus::Brightness, false, 0, 100, false);

  state = reduce(state, Input::Enter);
  CHECK(state.editingBrightness);
  for (int i = 0; i < 20; ++i) state = reduce(state, Input::Prev);
  CHECK(state.brightness == 10);
  state = reduce(state, Input::Next);
  CHECK(state.brightness == 20);
  state = reduce(state, Input::Escape);
  checkState(state, Screen::Controls, Focus::Brightness, false, 0, 20, false);
  state = reduce(state, Input::Escape);
  CHECK(state.focus == Focus::Count);

  state = reduce(state, Input::Enter);
  CHECK(state.pressCount == 1);
  CHECK(state.focus == Focus::Count);
  state = reduce(state, Input::None);
  CHECK(state.pressCount == 1);

  state = reduce(state, Input::Next);
  state = reduce(state, Input::Next);
  CHECK(state.focus == Focus::Orange);
  state = reduce(state, Input::Enter);
  CHECK(state.orangeMode);
  state = reduce(state, Input::Enter);
  CHECK(!state.orangeMode);
  state = reduce(state, Input::Next);
  state = reduce(state, Input::Enter);
  checkState(state, Screen::About, Focus::About, false, 1, 20, false);
  const State unchanged = reduce(state, Input::Next);
  checkState(unchanged, Screen::About, Focus::About, false, 1, 20, false);
  state = reduce(state, Input::Escape);
  checkState(state, Screen::Controls, Focus::About, false, 1, 20, false);
  state = reduce(reduce(state, Input::Enter), Input::Enter);
  CHECK(state.screen == Screen::Controls);
  CHECK(state.focus == Focus::About);
}

void checkTouch() {
  using namespace lvglbasics;
  State state = initialState();
  state = touch(state, TouchTarget::Count);
  checkState(state, Screen::Controls, Focus::Count, false, 1, 100, false);
  state = touch(state, TouchTarget::Brightness, -200);
  checkState(state, Screen::Controls, Focus::Brightness, false, 1, 10, false);
  state = touch(state, TouchTarget::Brightness, 55);
  CHECK(state.brightness == 55);
  state = touch(state, TouchTarget::Brightness, 500);
  CHECK(state.brightness == 100);
  state = touch(state, TouchTarget::Orange);
  CHECK(state.orangeMode && state.focus == Focus::Orange);
  state = touch(state, TouchTarget::About);
  CHECK(state.screen == Screen::About && state.focus == Focus::About);
  const State noCountOnAbout = touch(state, TouchTarget::Count);
  CHECK(noCountOnAbout.pressCount == state.pressCount);
  CHECK(noCountOnAbout.screen == Screen::About);
  state = touch(state, TouchTarget::Back);
  checkState(state, Screen::Controls, Focus::About, false, 1, 100, true);
  const State noBackOnControls = touch(state, TouchTarget::Back);
  CHECK(noBackOnControls.screen == Screen::Controls);
  CHECK(noBackOnControls.focus == Focus::About);
}

void checkValidityAndBrightness() {
  using namespace lvglbasics;
  CHECK(panelBrightness(10) == 26);
  CHECK(panelBrightness(50) == 128);
  CHECK(panelBrightness(100) == 255);
  CHECK(panelBrightness(-1) == 26);
  CHECK(panelBrightness(1000) == 255);
  State state = initialState();
  state.brightness = 9; CHECK(!valid(state));
  state = initialState(); state.brightness = 101; CHECK(!valid(state));
  state = initialState(); state.pressCount = -1; CHECK(!valid(state));
  state = initialState(); state.editingBrightness = true; CHECK(!valid(state));
  state.focus = Focus::Brightness; CHECK(valid(state));
  state.screen = Screen::About; CHECK(!valid(state));
  state = initialState(); state.screen = static_cast<Screen>(9); CHECK(!valid(state));
  state = initialState(); state.focus = static_cast<Focus>(9); CHECK(!valid(state));
}

void checkLayout() {
  using namespace lvglbasics;
  const Rect expected[] = {
    {104, 104, 260, 50}, {104, 200, 260, 42},
    {272, 266, 92, 50}, {104, 330, 260, 50},
  };
  for (size_t i = 0; i < kControlCount; ++i) {
    const auto rect = controlBounds(static_cast<Focus>(i));
    CHECK(rect.x == expected[i].x);
    CHECK(rect.y == expected[i].y);
    CHECK(rect.width == expected[i].width);
    CHECK(rect.height == expected[i].height);
    CHECK(rectInsideRoundFace(rect));
    CHECK(rectInsideRoundFace(rect, kFocusInset));
  }
  CHECK(!rectInsideRoundFace({0, 0, 20, 20}));
  CHECK(!rectInsideRoundFace({100, 100, 0, 20}));
  CHECK(!rectInsideRoundFace({100, 100, 20, -1}));
  CHECK(!rectInsideRoundFace({100, 100, 20, 20}, -1));
}

void checkGeneratedSequences() {
  using namespace lvglbasics;
  uint32_t random = 0x08092026u;
  for (int trial = 0; trial < 100; ++trial) {
    State first = initialState();
    State second = initialState();
    for (int step = 0; step < 200; ++step) {
      random = random * 1664525u + 1013904223u;
      const Input input = static_cast<Input>(random % 5);
      first = reduce(first, input);
      second = reduce(second, input);
      CHECK(valid(first));
      CHECK(first.screen == second.screen);
      CHECK(first.focus == second.focus);
      CHECK(first.editingBrightness == second.editingBrightness);
      CHECK(first.pressCount == second.pressCount);
      CHECK(first.brightness == second.brightness);
      CHECK(first.orangeMode == second.orangeMode);
    }
  }
}

}  // namespace

int main() {
  checkConstantsAndDefaults();
  checkButtons();
  checkTouch();
  checkValidityAndBrightness();
  checkLayout();
  checkGeneratedSequences();
  if (failures) {
    std::cerr << failures << " of " << assertions << " assertions failed\n";
    return 1;
  }
  std::cout << "LVGL basics contract: " << assertions << " assertions passed.\n";
  return 0;
}
