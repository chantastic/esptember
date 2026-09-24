#include <assert.h>
#include <stdint.h>
#include <iostream>
#include <vector>

#include "session.h"

using c25k::Session;
using c25k::SessionEvent;
using c25k::SegmentType;

static std::vector<SessionEvent> drain(Session& session) {
  std::vector<SessionEvent> events;
  SessionEvent event;
  while (session.popEvent(event)) events.push_back(event);
  return events;
}

static void checkProgram() {
  const uint32_t totals[27] = {
      1800, 1800, 1800, 1860, 1860, 1860, 1680, 1680, 1680,
      1890, 1890, 1890, 1860, 1860, 1800, 2040, 1980, 2100,
      2100, 2100, 2100, 2280, 2280, 2280, 2400, 2400, 2400};
  const uint32_t runs[27] = {
      480, 480, 480, 540, 540, 540, 540, 540, 540,
      960, 960, 960, 900, 960, 1200, 1080, 1200, 1500,
      1500, 1500, 1500, 1680, 1680, 1680, 1800, 1800, 1800};
  const uint8_t counts[27] = {
      18, 18, 18, 14, 14, 14, 10, 10, 10, 9, 9, 9,
      7, 5, 3, 7, 5, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};

  for (uint8_t i = 0; i < c25k::kWorkoutCount; ++i) {
    const auto& workout = c25k::workoutAt(i);
    assert(c25k::totalSeconds(workout) == totals[i]);
    assert(c25k::runSeconds(workout) == runs[i]);
    assert(workout.count == counts[i]);
    assert(workout.segments[0].type == SegmentType::Warmup);
    assert(workout.segments[0].seconds == 300);
    assert(workout.segments[workout.count - 1].type == SegmentType::Cooldown);
    assert(workout.segments[workout.count - 1].seconds == 300);
    for (uint8_t j = 1; j + 1 < workout.count; ++j) {
      assert(workout.segments[j].type == (j % 2 ? SegmentType::Run : SegmentType::Walk));
      assert(workout.segments[j].seconds >= 60);
    }

    // One very late update must cross every boundary with exact attribution,
    // stopping at completion instead of counting time after the workout.
    Session session;
    session.start(i, 12345);
    session.update(12345 + totals[i] * 1000 + 500000);
    assert(!session.active && !session.paused && !session.partial);
    assert(session.totalElapsedMs == totals[i] * 1000);
    assert(session.runElapsedMs == runs[i] * 1000);
    assert(session.remainingMs() == 0);
    assert(session.progressMs() == totals[i] * 1000);
    assert(session.completedSegments == (UINT32_C(1) << workout.count) - 1U);
    const auto events = drain(session);
    assert(events.size() == static_cast<size_t>(workout.count) * 2 + 1);
    assert(events.front() == SessionEvent::Walk);
    assert(events.back() == SessionEvent::Complete);
    unsigned warnings = 0;
    unsigned complete = 0;
    for (auto event : events) {
      warnings += event == SessionEvent::Warning;
      complete += event == SessionEvent::Complete;
    }
    assert(warnings == workout.count && complete == 1);
    session.update(9999999);
    assert(session.totalElapsedMs == totals[i] * 1000);
    assert(drain(session).empty());
  }
}

static void checkBoundaryAndWarning() {
  Session session;
  session.start(0, 100);
  assert(drain(session) == std::vector<SessionEvent>{SessionEvent::Walk});
  session.update(290099);
  assert(session.remainingMs() == 10001);
  assert(drain(session).empty());
  session.update(290100);
  assert(drain(session) == std::vector<SessionEvent>{SessionEvent::Warning});
  session.update(290101);
  assert(drain(session).empty());
  session.update(300100);
  assert(session.segmentIndex == 1 && session.segmentElapsedMs == 0);
  assert(session.runElapsedMs == 0);
  assert(drain(session) == std::vector<SessionEvent>{SessionEvent::Run});
  session.update(370100);  // Full first run, then ten seconds walking.
  assert(session.segmentIndex == 2 && session.segmentElapsedMs == 10000);
  assert(session.totalElapsedMs == 370000 && session.runElapsedMs == 60000);
  const std::vector<SessionEvent> expected = {SessionEvent::Warning, SessionEvent::Walk};
  assert(drain(session) == expected);
}

static void checkPauseAndControls() {
  Session session;
  session.start(0, 0);
  session.togglePause(310000);
  assert(session.paused && session.runElapsedMs == 10000);
  session.update(500000);
  assert(session.segmentElapsedMs == 10000 && session.totalElapsedMs == 310000);
  session.next(600000);
  assert(session.paused && session.partial && session.segmentIndex == 2);
  session.prev(600200);  // Restart walk.
  assert(session.paused && session.segmentIndex == 2);
  session.prev(600400);  // Track back to run while remaining paused.
  assert(session.paused && session.segmentIndex == 1);
  session.togglePause(700000);
  session.update(705000);
  assert(session.runElapsedMs == 15000 && session.totalElapsedMs == 315000);
  assert(session.segmentElapsedMs == 5000);
  session.prev(705100);  // Resume reset the chain: restart current run.
  assert(session.segmentIndex == 1 && session.segmentElapsedMs == 0);
  assert(session.runElapsedMs == 15100 && session.totalElapsedMs == 315100);
}

static void checkTrackBack() {
  Session session;
  session.start(0, 0);
  session.update(520000);  // Segment 4, ten seconds into a walk.
  assert(session.segmentIndex == 4);
  session.prev(520100);
  assert(session.segmentIndex == 4 && session.segmentElapsedMs == 0);
  session.prev(522100);  // Inclusive two-second chain boundary.
  assert(session.segmentIndex == 3 && session.segmentElapsedMs == 0);
  session.prev(522200);
  assert(session.segmentIndex == 2);
  session.prev(522300);
  assert(session.segmentIndex == 1);
  session.prev(522400);
  assert(session.segmentIndex == 0);
  session.prev(522500);  // At the beginning, chained extra presses are no-ops.
  assert(session.segmentIndex == 0 && session.segmentElapsedMs == 100);
  session.prev(524501);  // Outside the chain: restart the warm-up.
  assert(session.segmentElapsedMs == 0);
  assert(!session.partial);

  Session reset;
  reset.start(0, 0);
  reset.update(310000);
  reset.prev(310100);
  reset.next(310200);
  reset.prev(310300);  // Next broke chain, so remain on the walk.
  assert(reset.segmentIndex == 2);
  reset.prev(310400);
  assert(reset.segmentIndex == 1);
  reset.resetPrevChain();
  reset.prev(310500);
  assert(reset.segmentIndex == 1);
}

static void checkReplayAndCompletion() {
  Session session;
  session.start(14, 0);  // 5-min walk, 20-min run, 5-min walk.
  session.update(600000);
  session.prev(600000);  // Replay five minutes of running.
  session.update(2100000);
  assert(!session.active && !session.partial);
  assert(session.totalElapsedMs == 2100000 && session.runElapsedMs == 1500000);

  session.start(0, 3000000);
  session.next(3001000);  // One second of warm-up, then skip every segment.
  session.next(3002500);  // 1.5 seconds of first run.
  for (uint8_t i = session.segmentIndex; i < c25k::workoutAt(0).count; ++i) {
    session.next(3002500);
  }
  assert(!session.active && session.partial);
  assert(session.totalElapsedMs == 2500 && session.runElapsedMs == 1500);
  const auto events = drain(session);
  assert(events.back() == SessionEvent::Complete);

  session.start(26, 4000000);
  session.togglePause(4001000);
  session.next(4002000);
  session.next(4003000);
  session.next(4004000);
  assert(!session.active && !session.paused && session.partial);
  assert(session.totalElapsedMs == 1000 && session.runElapsedMs == 0);
}

static void checkWrapAndCancel() {
  Session session;
  const uint32_t start = UINT32_MAX - 999U;
  session.start(0, start);
  session.update(1000);
  assert(session.totalElapsedMs == 2000 && session.segmentElapsedMs == 2000);
  session.togglePause(2000);
  session.togglePause(9000);
  session.update(10000);
  assert(session.totalElapsedMs == 4000 && session.segmentElapsedMs == 4000);

  session.start(0, UINT32_MAX - 400000U);
  session.update(UINT32_MAX - 80000U);
  session.prev(UINT32_MAX - 1000U);
  const uint8_t original = session.segmentIndex;
  session.prev(500);
  assert(session.segmentIndex == original - 1);

  session.cancel();
  assert(!session.active && !session.paused && drain(session).empty());
  const uint64_t elapsed = session.totalElapsedMs;
  session.update(1000000);
  session.next(1000001);
  session.prev(1000002);
  session.togglePause(1000003);
  assert(session.totalElapsedMs == elapsed && drain(session).empty());
  session.start(1, 2000000);
  assert(session.workoutIndex == 1 && session.active && !session.partial);
  assert(session.totalElapsedMs == 0 && session.runElapsedMs == 0);
  assert(drain(session) == std::vector<SessionEvent>{SessionEvent::Walk});
}

static void checkLongReplayTotalsAndQueue() {
  Session session;
  session.start(26, 0);
  uint32_t now = 300000;
  session.update(now);
  // Repeated RUN restarts can exceed uint16 log seconds and the millis clock.
  // Actual counters remain 64-bit; persistence chooses its saturation policy.
  for (unsigned i = 0; i < 3000; ++i) {
    now += 1500000U;
    session.prev(now);
    assert(session.segmentIndex == 1 && !session.partial);
  }
  assert(session.totalElapsedMs == UINT64_C(4500300000));
  assert(session.runElapsedMs == UINT64_C(4500000000));
  now += 2100000U;
  session.update(now);
  assert(!session.active && !session.partial);
  assert(session.totalElapsedMs == UINT64_C(4502400000));
  assert(session.runElapsedMs == UINT64_C(4501800000));
  const auto events = drain(session);
  assert(events.size() == 64 && events.back() == SessionEvent::Complete);
}

static void checkSegmentCompletion() {
  Session session;
  session.start(0, 0);
  assert(session.completedSegments == 0);
  session.update(299999);
  assert(session.completedSegments == 0);
  session.update(300000);
  assert(session.segmentIndex == 1 && session.completedSegments == 1U);

  session.togglePause(301000);
  session.update(500000);  // Paused time cannot complete the run.
  assert(session.completedSegments == 1U);
  session.next(500000);  // The skipped run remains incomplete.
  assert(session.segmentIndex == 2 && session.completedSegments == 1U);
  session.togglePause(500000);
  session.update(590000);  // Finish the walk after the skipped run.
  assert(session.segmentIndex == 3 && session.completedSegments == 5U);

  session.prev(590000);  // Restart the current, unfinished segment.
  session.prev(590000);  // Revisit the already completed walk.
  assert(session.segmentIndex == 2 && session.completedSegments == 5U);
  session.prev(590000);  // Revisit the skipped run.
  assert(session.segmentIndex == 1 && session.completedSegments == 5U);
  session.update(649999);
  assert(session.completedSegments == 5U);
  session.update(650000);  // Finishing its replay now completes the run.
  assert(session.segmentIndex == 2 && session.completedSegments == 7U);
  session.next(650000);  // Skipping a completed segment preserves its bit.
  assert(session.segmentIndex == 3 && session.completedSegments == 7U);
  assert(session.partial);  // Replay does not remove the session's skip flag.

  session.start(26, 700000);
  assert(session.completedSegments == 0);
  session.update(2800000);  // Finish warm-up and run, but not cool-down.
  assert(session.segmentIndex == 2 && session.completedSegments == 3U);
  session.next(2800000);  // Next on the final segment must not fill its circle.
  assert(!session.active && session.partial && session.completedSegments == 3U);
  session.update(3100000);
  assert(session.completedSegments == 3U);

  session.start(0, 3200000);
  for (uint8_t i = 0; i < c25k::workoutAt(0).count; ++i) session.next(3200000);
  assert(!session.active && session.partial && session.completedSegments == 0);

  session.start(0, 4000000);
  session.next(4360000);  // Account for natural boundaries before applying Next.
  assert(session.segmentIndex == 3 && session.completedSegments == 3U);
  session.cancel();
  assert(session.completedSegments == 0);
}

int main() {
  checkProgram();
  checkBoundaryAndWarning();
  checkPauseAndControls();
  checkTrackBack();
  checkReplayAndCompletion();
  checkWrapAndCancel();
  checkLongReplayTotalsAndQueue();
  checkSegmentCompletion();
  std::cout << "C25K program/session checks passed (27 workouts, timing, controls, cues, wrap, segment completion).\n";
}
