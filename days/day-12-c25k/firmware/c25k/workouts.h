#pragma once

#include <stdint.h>

namespace c25k {

enum class SegmentType : uint8_t { Warmup, Run, Walk, Cooldown };

struct Segment {
  SegmentType type;
  uint16_t seconds;
};

struct Workout {
  const Segment* segments;
  uint8_t count;
};

constexpr uint8_t kWorkoutCount = 27;

namespace program {
// Repeated days share immutable segment data. Each of the 27 workout slots has
// its own descriptor, and warm-up/cool-down are ordinary, skippable segments.
constexpr Segment week1[] = {
    {SegmentType::Warmup, 300},
    {SegmentType::Run, 60}, {SegmentType::Walk, 90},
    {SegmentType::Run, 60}, {SegmentType::Walk, 90},
    {SegmentType::Run, 60}, {SegmentType::Walk, 90},
    {SegmentType::Run, 60}, {SegmentType::Walk, 90},
    {SegmentType::Run, 60}, {SegmentType::Walk, 90},
    {SegmentType::Run, 60}, {SegmentType::Walk, 90},
    {SegmentType::Run, 60}, {SegmentType::Walk, 90},
    {SegmentType::Run, 60}, {SegmentType::Walk, 90},
    {SegmentType::Cooldown, 300}};

constexpr Segment week2[] = {
    {SegmentType::Warmup, 300},
    {SegmentType::Run, 90}, {SegmentType::Walk, 120},
    {SegmentType::Run, 90}, {SegmentType::Walk, 120},
    {SegmentType::Run, 90}, {SegmentType::Walk, 120},
    {SegmentType::Run, 90}, {SegmentType::Walk, 120},
    {SegmentType::Run, 90}, {SegmentType::Walk, 120},
    {SegmentType::Run, 90}, {SegmentType::Walk, 120},
    {SegmentType::Cooldown, 300}};

constexpr Segment week3[] = {
    {SegmentType::Warmup, 300},
    {SegmentType::Run, 90}, {SegmentType::Walk, 90},
    {SegmentType::Run, 180}, {SegmentType::Walk, 180},
    {SegmentType::Run, 90}, {SegmentType::Walk, 90},
    {SegmentType::Run, 180}, {SegmentType::Walk, 180},
    {SegmentType::Cooldown, 300}};

constexpr Segment week4[] = {
    {SegmentType::Warmup, 300},
    {SegmentType::Run, 180}, {SegmentType::Walk, 90},
    {SegmentType::Run, 300}, {SegmentType::Walk, 150},
    {SegmentType::Run, 180}, {SegmentType::Walk, 90},
    {SegmentType::Run, 300},
    {SegmentType::Cooldown, 300}};

constexpr Segment week5day1[] = {
    {SegmentType::Warmup, 300},
    {SegmentType::Run, 300}, {SegmentType::Walk, 180},
    {SegmentType::Run, 300}, {SegmentType::Walk, 180},
    {SegmentType::Run, 300},
    {SegmentType::Cooldown, 300}};

constexpr Segment week5day2[] = {
    {SegmentType::Warmup, 300},
    {SegmentType::Run, 480}, {SegmentType::Walk, 300},
    {SegmentType::Run, 480},
    {SegmentType::Cooldown, 300}};

constexpr Segment week5day3[] = {
    {SegmentType::Warmup, 300}, {SegmentType::Run, 1200},
    {SegmentType::Cooldown, 300}};

constexpr Segment week6day1[] = {
    {SegmentType::Warmup, 300},
    {SegmentType::Run, 300}, {SegmentType::Walk, 180},
    {SegmentType::Run, 480}, {SegmentType::Walk, 180},
    {SegmentType::Run, 300},
    {SegmentType::Cooldown, 300}};

constexpr Segment week6day2[] = {
    {SegmentType::Warmup, 300},
    {SegmentType::Run, 600}, {SegmentType::Walk, 180},
    {SegmentType::Run, 600},
    {SegmentType::Cooldown, 300}};

constexpr Segment run25[] = {
    {SegmentType::Warmup, 300}, {SegmentType::Run, 1500},
    {SegmentType::Cooldown, 300}};

constexpr Segment run28[] = {
    {SegmentType::Warmup, 300}, {SegmentType::Run, 1680},
    {SegmentType::Cooldown, 300}};

constexpr Segment run30[] = {
    {SegmentType::Warmup, 300}, {SegmentType::Run, 1800},
    {SegmentType::Cooldown, 300}};

template <unsigned N>
constexpr Workout makeWorkout(const Segment (&segments)[N]) {
  return {segments, static_cast<uint8_t>(N)};
}

constexpr Workout workouts[kWorkoutCount] = {
    makeWorkout(week1), makeWorkout(week1), makeWorkout(week1),
    makeWorkout(week2), makeWorkout(week2), makeWorkout(week2),
    makeWorkout(week3), makeWorkout(week3), makeWorkout(week3),
    makeWorkout(week4), makeWorkout(week4), makeWorkout(week4),
    makeWorkout(week5day1), makeWorkout(week5day2), makeWorkout(week5day3),
    makeWorkout(week6day1), makeWorkout(week6day2), makeWorkout(run25),
    makeWorkout(run25), makeWorkout(run25), makeWorkout(run25),
    makeWorkout(run28), makeWorkout(run28), makeWorkout(run28),
    makeWorkout(run30), makeWorkout(run30), makeWorkout(run30)};
}  // namespace program

inline const Workout& workoutAt(uint8_t index) {
  return program::workouts[index < kWorkoutCount ? index : kWorkoutCount - 1];
}

inline uint32_t totalSeconds(const Workout& workout) {
  uint32_t total = 0;
  for (uint8_t i = 0; i < workout.count; ++i) total += workout.segments[i].seconds;
  return total;
}

inline uint32_t runSeconds(const Workout& workout) {
  uint32_t total = 0;
  for (uint8_t i = 0; i < workout.count; ++i) {
    if (workout.segments[i].type == SegmentType::Run) total += workout.segments[i].seconds;
  }
  return total;
}

}  // namespace c25k
