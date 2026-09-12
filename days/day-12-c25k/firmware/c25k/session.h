#pragma once

#include "workouts.h"

namespace c25k {

enum class SessionEvent : uint8_t { Run, Walk, Warning, Pause, Resume, Complete };

class Session {
 public:
  uint8_t workoutIndex = 0;
  uint8_t segmentIndex = 0;
  bool active = false;
  bool paused = false;
  bool partial = false;
  uint32_t segmentElapsedMs = 0;
  uint64_t totalElapsedMs = 0;
  uint64_t runElapsedMs = 0;

  void start(uint8_t index, uint32_t now) {
    workoutIndex = index < kWorkoutCount ? index : kWorkoutCount - 1;
    segmentIndex = 0;
    active = true;
    paused = partial = false;
    segmentElapsedMs = 0;
    totalElapsedMs = runElapsedMs = 0;
    lastUpdateMs_ = now;
    warningFired_ = false;
    resetPrevChain();
    clearEvents();
    emitSegment();
  }

  // Unsigned subtraction survives millis() rollover. Calls must be less than
  // one complete 32-bit millis period apart (about 49.7 days).
  void update(uint32_t now) {
    uint32_t delta = now - lastUpdateMs_;
    lastUpdateMs_ = now;
    if (!active || paused) return;

    // Consume time at each boundary so a delayed loop still attributes RUN
    // time correctly. Any time after natural completion is ignored.
    while (active && delta > 0) {
      const uint32_t length = static_cast<uint32_t>(currentSegment().seconds) * 1000U;
      const uint32_t remaining = length - segmentElapsedMs;
      const uint32_t consumed = delta < remaining ? delta : remaining;
      segmentElapsedMs += consumed;
      totalElapsedMs += consumed;
      if (currentSegment().type == SegmentType::Run) runElapsedMs += consumed;
      delta -= consumed;

      if (!warningFired_ && segmentElapsedMs >= length - 10000U) {
        warningFired_ = true;
        emit(SessionEvent::Warning);
      }
      if (segmentElapsedMs == length) advance();
    }
  }

  void next(uint32_t now) {
    update(now);
    resetPrevChain();
    if (!active) return;
    partial = true;
    advance();
  }

  void prev(uint32_t now) {
    update(now);
    if (!active) return;
    const bool chained = prevChain_ && static_cast<uint32_t>(now - lastPrevMs_) <= 2000U;
    prevChain_ = true;
    lastPrevMs_ = now;
    if (chained) {
      if (segmentIndex == 0) return;
      --segmentIndex;
    }
    segmentElapsedMs = 0;
    warningFired_ = false;
    emitSegment();
  }

  void togglePause(uint32_t now) {
    update(now);
    resetPrevChain();
    if (!active) return;
    paused = !paused;
    emit(paused ? SessionEvent::Pause : SessionEvent::Resume);
  }

  void cancel() {
    active = paused = false;
    resetPrevChain();
    clearEvents();
  }

  // Call for other acknowledged actions, e.g. opening/dismissing Cancel Confirm.
  void resetPrevChain() { prevChain_ = false; }

  const Segment& currentSegment() const {
    return workoutAt(workoutIndex).segments[segmentIndex];
  }

  uint32_t remainingMs() const {
    return static_cast<uint32_t>(currentSegment().seconds) * 1000U - segmentElapsedMs;
  }

  // Position in the planned session for the outer progress arc. Actual active
  // elapsed time is separate and includes every replay after track-back.
  uint32_t progressMs() const {
    const Workout& workout = workoutAt(workoutIndex);
    uint32_t position = segmentElapsedMs;
    for (uint8_t i = 0; i < segmentIndex; ++i) {
      position += static_cast<uint32_t>(workout.segments[i].seconds) * 1000U;
    }
    return position;
  }

  // Drain each loop into an asynchronous cue player. 64 events can hold an
  // entire W1 session (start + 18 warnings + 17 transitions + completion).
  // If a caller fails to drain between many controls, oldest cues are dropped;
  // newest state and completion always remain available. No dynamic allocation.
  bool popEvent(SessionEvent& event) {
    if (eventCount_ == 0) return false;
    event = events_[eventHead_];
    eventHead_ = static_cast<uint8_t>((eventHead_ + 1U) % kEventCapacity);
    --eventCount_;
    return true;
  }

  void clearEvents() { eventHead_ = eventCount_ = 0; }

 private:
  static constexpr uint8_t kEventCapacity = 64;
  SessionEvent events_[kEventCapacity] = {};
  uint8_t eventHead_ = 0;
  uint8_t eventCount_ = 0;
  uint32_t lastUpdateMs_ = 0;
  uint32_t lastPrevMs_ = 0;
  bool prevChain_ = false;
  bool warningFired_ = false;

  void emit(SessionEvent event) {
    if (eventCount_ == kEventCapacity) {
      eventHead_ = static_cast<uint8_t>((eventHead_ + 1U) % kEventCapacity);
      --eventCount_;
    }
    events_[(eventHead_ + eventCount_) % kEventCapacity] = event;
    ++eventCount_;
  }

  void emitSegment() {
    emit(currentSegment().type == SegmentType::Run ? SessionEvent::Run : SessionEvent::Walk);
  }

  void advance() {
    if (segmentIndex + 1U >= workoutAt(workoutIndex).count) {
      // Mark the nominal position complete even if Next skipped the final part.
      segmentElapsedMs = static_cast<uint32_t>(currentSegment().seconds) * 1000U;
      active = paused = false;
      resetPrevChain();
      emit(SessionEvent::Complete);
      return;
    }
    ++segmentIndex;
    segmentElapsedMs = 0;
    warningFired_ = false;
    emitSegment();
  }
};

}  // namespace c25k
