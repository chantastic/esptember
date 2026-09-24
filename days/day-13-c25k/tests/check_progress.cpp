#include "progress.h"
#include <cassert>
#include <cstdio>
using namespace c25k;
int main() {
  Progress p{}; p.defaults(); assert(p.valid()); assert(!p.newest(0));
  p.append({14, 1, 1860, 1200, 0}); assert(p.cursor == 15);
  p.append({3, 2, 100, 20, 1}); assert(p.cursor == 4);
  assert(p.newest(0)->flags == 1 && p.newest(1)->workout == 14);
  p.append({26, 3, 2400, 1800, 0});
  assert(p.cursor == 26 && p.program_complete);
  p.append({0, 4, 1800, 480, 0}); assert(!p.program_complete);
  for (uint32_t i = 5; i < 400; ++i) p.append({0, i, 1800, 480, 0});
  assert(p.count == 256 && p.valid());
  assert(p.newest(0)->timestamp == 399 && p.newest(255)->timestamp == 144);
  assert(!p.newest(256));
  Progress roundtrip; memcpy(&roundtrip, &p, sizeof(p)); assert(roundtrip.valid());
  roundtrip.cursor ^= 1; assert(!roundtrip.valid());
  p.sound_on = 0; p.resetProgress();
  assert(p.valid() && p.cursor == 0 && !p.count && !p.sound_on && p.vib_on);

  Progress saved{}; saved.defaults();
  saved.sound_on = 0; saved.vib_on = 1;
  saved.append({4, 100, 1200, 600, 1});
  saved.append({26, 200, 2400, 1800, 0});
  assert(saved.valid() && saved.program_complete && saved.cursor == 26);
  const Progress beforeReset = saved;
  int writes = 0;
  const auto inspectReset = [&](const Progress& candidate) {
    ++writes;
    assert(candidate.valid() && candidate.cursor == 0 && candidate.count == 0 &&
           candidate.head == 0 && !candidate.program_complete);
    assert(candidate.sound_on == beforeReset.sound_on &&
           candidate.vib_on == beforeReset.vib_on);
    for (const auto& entry : candidate.entries) {
      const LogEntry empty{};
      assert(memcmp(&entry, &empty, sizeof(entry)) == 0);
    }
    // The live progress remains intact while persistence is attempted.
    assert(memcmp(&saved, &beforeReset, sizeof(saved)) == 0);
  };
  assert(!resetSavedProgress(saved, [&](const Progress& candidate) {
    inspectReset(candidate);
    return false;
  }));
  assert(writes == 1);
  assert(memcmp(&saved, &beforeReset, sizeof(saved)) == 0);

  Progress persisted{};
  writes = 0;
  assert(resetSavedProgress(saved, [&](const Progress& candidate) {
    inspectReset(candidate);
    persisted = candidate;
    return true;
  }));
  assert(writes == 1 && saved.valid());
  assert(memcmp(&saved, &persisted, sizeof(saved)) == 0);
  assert(saved.count == 0 && saved.cursor == 0 && !saved.program_complete);

  assert(logSeconds(999) == 0 && logSeconds(123456) == 123);
  assert(logSeconds(999999999) == 65535);
  puts("PASS progress: ring overwrite, repeats, cursor, transactional reset, corruption, duration limits");
}
