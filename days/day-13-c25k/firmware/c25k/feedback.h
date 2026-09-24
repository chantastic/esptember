#pragma once
#include <M5Unified.h>
#include "session.h"

namespace c25k {
class Feedback {
 public:
  void begin() {
    // Prepare the codec once so a segment cue doesn't initialize I2S mid-run.
    speakerReady = M5.Speaker.begin();
    M5.Speaker.setVolume(180);
    M5.Power.setVibration(0);
  }
  void enable(bool sound, bool vibration) {
    soundOn = sound; vibrationOn = vibration;
    if (!soundOn) M5.Speaker.stop();
    if (!vibrationOn) M5.Power.setVibration(0);
  }
  void stop() {
    count = 0; position = 0;
    M5.Power.setVibration(0); M5.Speaker.stop();
  }
  void tap(uint32_t now) {
    if (position < count) return;
    const Step steps[] = {{22, 0, 150}};
    play(steps, 1, now);
  }
  void cue(SessionEvent event, uint32_t now) {
    switch (event) {
      case SessionEvent::Run: {
        const Step s[] = {{130, 660, 230}, {95, 0, 0}, {150, 880, 230}};
        play(s, 3, now); break;
      }
      case SessionEvent::Walk: {
        const Step s[] = {{170, 550, 230}, {190, 390, 230}};
        play(s, 2, now); break;
      }
      case SessionEvent::Warning: {
        const Step s[] = {{45, 1200, 170}}; play(s, 1, now); break;
      }
      case SessionEvent::Pause: {
        const Step s[] = {{85, 330, 200}}; play(s, 1, now); break;
      }
      case SessionEvent::Resume: {
        const Step s[] = {{85, 880, 200}}; play(s, 1, now); break;
      }
      case SessionEvent::Complete: {
        const Step s[] = {{110, 523, 230}, {80, 0, 0}, {110, 659, 230},
                          {80, 0, 0}, {160, 784, 230}};
        play(s, 5, now); break;
      }
    }
  }
  void update(uint32_t now) {
    if (position >= count || uint32_t(now - started) < steps[position].ms) return;
    ++position;
    if (position == count) { M5.Power.setVibration(0); return; }
    started = now; apply();
  }
  bool speakerReady = false;
 private:
  struct Step { uint16_t ms, hz; uint8_t level; };
  Step steps[5]{};
  uint8_t count = 0, position = 0;
  uint32_t started = 0;
  bool soundOn = true, vibrationOn = true;
  void play(const Step* pattern, uint8_t length, uint32_t now) {
    M5.Speaker.stop();
    for (uint8_t i = 0; i < length; ++i) steps[i] = pattern[i];
    count = length; position = 0; started = now; apply();
  }
  void apply() {
    const Step& step = steps[position];
    M5.Power.setVibration(vibrationOn ? step.level : 0);
    if (soundOn && speakerReady && step.hz)
      M5.Speaker.tone(step.hz, step.ms, 0, true);
  }
};
}  // namespace c25k
