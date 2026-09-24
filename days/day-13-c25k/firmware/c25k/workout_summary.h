#pragma once

#include "workouts.h"

namespace c25k {

struct WorkoutSummary {
  uint8_t firstCore;
  uint8_t coreCount;
  uint8_t patternCount;
  uint8_t repeats;
  uint16_t warmupSeconds;
  uint16_t cooldownSeconds;
  uint32_t runSeconds;
  uint32_t walkSeconds;
};

// References the original segment table: no separate workout descriptions need
// to stay in sync. Only a leading Warmup and trailing Cooldown are trimmed.
// Run/walk totals cover the core, so walkSeconds excludes warm-up and cool-down.
// The core is patternCount consecutive segments starting at firstCore, repeated
// repeats times. An empty core has patternCount == repeats == 0.
inline WorkoutSummary summarizeWorkout(const Workout& workout) {
  WorkoutSummary summary{};
  uint8_t end = workout.count;
  if (end && workout.segments[0].type == SegmentType::Warmup) {
    summary.warmupSeconds = workout.segments[0].seconds;
    summary.firstCore = 1;
  }
  if (end > summary.firstCore &&
      workout.segments[end - 1].type == SegmentType::Cooldown) {
    summary.cooldownSeconds = workout.segments[--end].seconds;
  }
  summary.coreCount = end - summary.firstCore;
  if (!summary.coreCount) return summary;

  const Segment* core = workout.segments + summary.firstCore;
  for (uint16_t i = 0; i < summary.coreCount; ++i) {
    if (core[i].type == SegmentType::Run) summary.runSeconds += core[i].seconds;
    if (core[i].type == SegmentType::Walk) summary.walkSeconds += core[i].seconds;
  }

  // A partial final repetition is not shortened: every displayed repeat must
  // reproduce both the activity and duration of every original core segment.
  for (uint16_t length = 1; length <= summary.coreCount; ++length) {
    if (summary.coreCount % length) continue;
    bool matches = true;
    for (uint16_t i = length; i < summary.coreCount; ++i) {
      if (core[i].type != core[i % length].type ||
          core[i].seconds != core[i % length].seconds) {
        matches = false;
        break;
      }
    }
    if (matches) {
      summary.patternCount = static_cast<uint8_t>(length);
      summary.repeats = static_cast<uint8_t>(summary.coreCount / length);
      break;
    }
  }
  return summary;
}

}  // namespace c25k
