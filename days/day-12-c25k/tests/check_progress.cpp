#include "../firmware/c25k/progress.h"
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
  assert(logSeconds(999) == 0 && logSeconds(123456) == 123);
  assert(logSeconds(999999999) == 65535);
  puts("PASS progress: ring overwrite, repeats, cursor, reset, corruption, duration limits");
}
