#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include "touch_map.h"

int main() {
  using namespace touchmap;
  const float coefficients[6] = {2, 0, 1, 0, 2, -1};
  const auto v1 = makeV1(coefficients);
  assert(sizeof(v1) == 40 && valid(v1));
  assert(checksum(v1) == v1.checksum);
  assert(std::fabs(apply(v1, {12, 20}).x - 25) < 0.001f);
  assert(std::fabs(apply(v1, {12, 20}).y - 39) < 0.001f);
  auto tamperedV1 = v1;
  tamperedV1.coefficients[2] += 1;
  assert(!valid(tamperedV1));
  auto v2 = makeV2(coefficients, 7);
  assert(sizeof(v2) == 440 && valid(v2));
  assert(v2.generation == 7);
  assert(std::fabs(apply(v2, {12, 20}).x - 25) < 0.001f);
  v2.centerResidual[0] = 10;
  v2.checksum = checksum(v2);
  assert(valid(v2));
  const Point centerRaw = {(234.0f - 1) / 2, (233.0f + 1) / 2};
  assert(std::fabs(apply(v2, centerRaw).x - 244) < 0.001f);
  v2.residuals[0][0][0] = 40;
  v2.checksum = checksum(v2);
  assert(!valid(v2));

  Point samples[16];
  for (auto& sample : samples) sample = {100, 150};
  Point median;
  assert(stableMedian(samples, 16, 80, &median));
  assert(median.x == 100 && median.y == 150);
  assert(!stableMedian(samples, 15, 80, &median));
  assert(!stableMedian(samples, 16, 79, &median));
  samples[15].x = 120;
  assert(!stableMedian(samples, 16, 80, &median));

  const Point raw[5] = {{0,0},{0,100},{100,0},{100,100},{50,50}};
  Point target[5];
  for (int i = 0; i < 5; ++i) target[i] = apply(v1, raw[i]);
  float fitted[6];
  assert(fitAffine(raw, target, 5, fitted));
  for (int i = 0; i < 6; ++i)
    assert(std::fabs(fitted[i] - coefficients[i]) < 0.001f);
  const Point singular[5] = {{1,1},{1,1},{1,1},{1,1},{1,1}};
  assert(!fitAffine(singular, target, 5, fitted));
}
