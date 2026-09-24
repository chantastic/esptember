#include <cassert>
#include <cstdio>
#include <initializer_list>

#include "../firmware/c25k/workout_summary.h"

using namespace c25k;

static void checkReconstruction(const Workout& workout) {
  const WorkoutSummary summary = summarizeWorkout(workout);
  assert(summary.firstCore + summary.coreCount <= workout.count);
  if (!summary.coreCount) {
    assert(!summary.patternCount && !summary.repeats);
    assert(!summary.runSeconds && !summary.walkSeconds);
    return;
  }
  assert(summary.patternCount && summary.repeats);
  assert(summary.patternCount * summary.repeats == summary.coreCount);
  uint32_t runs = 0;
  uint32_t walks = 0;
  for (uint16_t i = 0; i < summary.coreCount; ++i) {
    const Segment& original = workout.segments[summary.firstCore + i];
    const Segment& reconstructed =
        workout.segments[summary.firstCore + i % summary.patternCount];
    assert(original.type == reconstructed.type);
    assert(original.seconds == reconstructed.seconds);
    if (original.type == SegmentType::Run) runs += original.seconds;
    if (original.type == SegmentType::Walk) walks += original.seconds;
  }
  assert(summary.runSeconds == runs && summary.walkSeconds == walks);
  // Verify that no shorter exact motif could describe the same core.
  for (uint16_t length = 1; length < summary.patternCount; ++length) {
    if (summary.coreCount % length) continue;
    bool differs = false;
    for (uint16_t i = length; i < summary.coreCount; ++i) {
      const Segment& a = workout.segments[summary.firstCore + i];
      const Segment& b = workout.segments[summary.firstCore + i % length];
      differs |= a.type != b.type || a.seconds != b.seconds;
    }
    assert(differs);
  }
}

static void checkProgram() {
  struct Expected {
    uint8_t patternCount;
    uint8_t repeats;
    uint32_t runSeconds;
    uint32_t walkSeconds;
  };
  const Expected expected[kWorkoutCount] = {
      {2, 8, 480, 720}, {2, 8, 480, 720}, {2, 8, 480, 720},
      {2, 6, 540, 720}, {2, 6, 540, 720}, {2, 6, 540, 720},
      {4, 2, 540, 540}, {4, 2, 540, 540}, {4, 2, 540, 540},
      {7, 1, 960, 330}, {7, 1, 960, 330}, {7, 1, 960, 330},
      {5, 1, 900, 360}, {3, 1, 960, 300}, {1, 1, 1200, 0},
      {5, 1, 1080, 360}, {3, 1, 1200, 180}, {1, 1, 1500, 0},
      {1, 1, 1500, 0}, {1, 1, 1500, 0}, {1, 1, 1500, 0},
      {1, 1, 1680, 0}, {1, 1, 1680, 0}, {1, 1, 1680, 0},
      {1, 1, 1800, 0}, {1, 1, 1800, 0}, {1, 1, 1800, 0}};
  for (uint8_t i = 0; i < kWorkoutCount; ++i) {
    const Workout& workout = workoutAt(i);
    const WorkoutSummary summary = summarizeWorkout(workout);
    assert(summary.firstCore == 1 && summary.coreCount == workout.count - 2);
    assert(summary.warmupSeconds == 300 && summary.cooldownSeconds == 300);
    assert(summary.patternCount == expected[i].patternCount);
    assert(summary.repeats == expected[i].repeats);
    assert(summary.runSeconds == expected[i].runSeconds);
    assert(summary.walkSeconds == expected[i].walkSeconds);
    assert(summary.runSeconds == runSeconds(workout));
    assert(summary.warmupSeconds + summary.runSeconds + summary.walkSeconds +
               summary.cooldownSeconds == totalSeconds(workout));
    checkReconstruction(workout);
  }
}

static void checkEmptyAndSingle() {
  const Workout empty{nullptr, 0};
  const auto summary = summarizeWorkout(empty);
  assert(!summary.firstCore && !summary.coreCount);
  assert(!summary.warmupSeconds && !summary.cooldownSeconds);
  checkReconstruction(empty);

  const Segment warmupOnly[] = {{SegmentType::Warmup, 123}};
  const Workout warmup = program::makeWorkout(warmupOnly);
  assert(summarizeWorkout(warmup).warmupSeconds == 123);
  assert(summarizeWorkout(warmup).firstCore == 1);
  checkReconstruction(warmup);

  const Segment cooldownOnly[] = {{SegmentType::Cooldown, 456}};
  const Workout cooldown = program::makeWorkout(cooldownOnly);
  assert(summarizeWorkout(cooldown).cooldownSeconds == 456);
  assert(!summarizeWorkout(cooldown).firstCore);
  checkReconstruction(cooldown);

  const Segment bookends[] = {
      {SegmentType::Warmup, 123}, {SegmentType::Cooldown, 456}};
  const Workout noCore = program::makeWorkout(bookends);
  assert(!summarizeWorkout(noCore).coreCount);
  assert(summarizeWorkout(noCore).warmupSeconds == 123);
  assert(summarizeWorkout(noCore).cooldownSeconds == 456);
  checkReconstruction(noCore);

  for (SegmentType type : {SegmentType::Run, SegmentType::Walk}) {
    const Segment single[] = {{type, 777}};
    const Workout workout = program::makeWorkout(single);
    const auto actual = summarizeWorkout(workout);
    assert(!actual.firstCore && actual.coreCount == 1);
    assert(actual.patternCount == 1 && actual.repeats == 1);
    assert(!actual.warmupSeconds && !actual.cooldownSeconds);
    assert(actual.runSeconds == (type == SegmentType::Run ? 777 : 0));
    assert(actual.walkSeconds == (type == SegmentType::Walk ? 777 : 0));
    checkReconstruction(workout);
  }
}

static void checkExactRepetition() {
  // Equal durations with different activities must retain the activity order.
  const Segment types[] = {
      {SegmentType::Run, 60}, {SegmentType::Walk, 60},
      {SegmentType::Run, 60}, {SegmentType::Walk, 60}};
  assert(summarizeWorkout(program::makeWorkout(types)).patternCount == 2);
  checkReconstruction(program::makeWorkout(types));

  // The same activity with different durations is also a distinct pattern.
  const Segment durations[] = {
      {SegmentType::Run, 60}, {SegmentType::Run, 90},
      {SegmentType::Run, 60}, {SegmentType::Run, 90}};
  assert(summarizeWorkout(program::makeWorkout(durations)).patternCount == 2);
  checkReconstruction(program::makeWorkout(durations));

  const Segment partial[] = {
      {SegmentType::Run, 60}, {SegmentType::Walk, 90}, {SegmentType::Run, 60}};
  assert(summarizeWorkout(program::makeWorkout(partial)).patternCount == 3);
  assert(summarizeWorkout(program::makeWorkout(partial)).repeats == 1);
  checkReconstruction(program::makeWorkout(partial));

  // Exercise the full descriptor count and totals larger than uint16_t.
  Segment largest[255];
  for (auto& segment : largest) segment = {SegmentType::Run, 65535};
  const Workout maximum = program::makeWorkout(largest);
  assert(summarizeWorkout(maximum).coreCount == 255);
  assert(summarizeWorkout(maximum).patternCount == 1);
  assert(summarizeWorkout(maximum).repeats == 255);
  assert(summarizeWorkout(maximum).runSeconds == 65535UL * 255);
  checkReconstruction(maximum);
  largest[254].seconds = 65534;
  assert(summarizeWorkout(maximum).patternCount == 255);
  assert(summarizeWorkout(maximum).repeats == 1);
  checkReconstruction(maximum);
}

int main() {
  checkProgram();
  checkEmptyAndSingle();
  checkExactRepetition();
  puts("PASS workout summary: all 27 workouts, exact motifs, totals, empty/single/max-size inputs");
}
