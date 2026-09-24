#pragma once

#include <cstdint>

namespace c25k {

enum class Input { None, Next, Prev, Enter, Escape };

// Takes M5Unified's debounced state and release-time wasClicked flags. A gesture
// owns its buttons until both are released, so a chord cannot leak a page turn.
class ButtonGrammar {
 public:
  Input update(uint32_t now, bool a, bool b, bool clickedA, bool clickedB) {
    const bool bothUp = !a && !b;
    if (mode_ == Mode::Idle) {
      if (bothUp) return Input::None;
      firstA_ = a;  // Same-sample presses tie to Yellow for a brief overlap only.
      firstDown_ = now;
      firstReleased_ = false;
      firstClicked_ = false;
      mode_ = a && b ? Mode::Together : Mode::Single;
      overlapStart_ = now;
      return Input::None;
    }

    const bool firstDown = firstA_ ? a : b;
    const bool otherDown = firstA_ ? b : a;
    const bool firstClick = firstA_ ? clickedA : clickedB;

    switch (mode_) {
      case Mode::Single:
        if (firstDown) {
          if (otherDown) {
            overlapStart_ = now;
            mode_ = elapsed(now, firstDown_) <= kJoinMs
                        ? Mode::Together : Mode::LateOverlap;
          }
          return Input::None;
        }
        // If the other button arrives in this release sample, no simultaneous
        // hold was observed. It cannot retroactively create a chord or a click.
        mode_ = bothUp ? Mode::Idle : Mode::Drain;
        return firstClick && !otherDown ? firstInput() : Input::None;

      case Mode::Together: {
        const uint32_t overlap = elapsed(now, overlapStart_);
        if (overlap >= kHoldMs) {
          // A release first observed at the threshold is still a hold, never
          // Enter. Release flags are swallowed even after staggered releases.
          mode_ = bothUp ? Mode::Idle : Mode::Drain;
          return Input::Escape;
        }
        if (a && b) return Input::None;
        if (overlap >= kOverlapMs) {
          mode_ = bothUp ? Mode::Idle : Mode::ChordRelease;
          return bothUp ? Input::Enter : Input::None;
        }
        mode_ = Mode::FallbackRelease;
        rememberFirstRelease(firstDown, firstClick);
        if (!bothUp) return Input::None;
        mode_ = Mode::Idle;
        return firstClicked_ ? firstInput() : Input::None;
      }

      case Mode::ChordRelease:
        if (!bothUp) return Input::None;
        mode_ = Mode::Idle;
        return Input::Enter;

      case Mode::FallbackRelease:
        // An overlap shorter than 80 ms falls back only to the first button's
        // real click, deferred until both are up. A single hold is never a click.
        rememberFirstRelease(firstDown, firstClick);
        if (!bothUp) return Input::None;
        mode_ = Mode::Idle;
        return firstClicked_ ? firstInput() : Input::None;

      case Mode::LateOverlap:
        // A late second button cannot upgrade into Enter/Escape. It is swallowed
        // until release, and the first click requires the other button to be up.
        if (firstDown) return Input::None;
        mode_ = bothUp ? Mode::Idle : Mode::Drain;
        return firstClick && !otherDown ? firstInput() : Input::None;

      case Mode::Drain:
        if (bothUp) mode_ = Mode::Idle;
        return Input::None;

      case Mode::Idle:
        break;
    }
    return Input::None;
  }

 private:
  enum class Mode { Idle, Single, Together, ChordRelease, FallbackRelease,
                    LateOverlap, Drain };
  static constexpr uint32_t kJoinMs = 80;
  static constexpr uint32_t kOverlapMs = 80;
  static constexpr uint32_t kHoldMs = 600;
  Mode mode_ = Mode::Idle;
  uint32_t firstDown_ = 0;
  uint32_t overlapStart_ = 0;
  bool firstA_ = false;
  bool firstReleased_ = false;
  bool firstClicked_ = false;

  static uint32_t elapsed(uint32_t now, uint32_t start) { return now - start; }
  Input firstInput() const { return firstA_ ? Input::Prev : Input::Next; }
  void rememberFirstRelease(bool firstDown, bool clicked) {
    if (!firstDown && !firstReleased_) {
      firstReleased_ = true;
      firstClicked_ = clicked;
    }
  }
};

}  // namespace c25k
