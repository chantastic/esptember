#include <cassert>
#include <cmath>
#include <cstdint>
#include "single_blob_motion.h"

int main() {
  using namespace singleblob;
  assert(clampGaze(-100, 15) == -15);
  assert(clampGaze(100, 15) == 15);
  assert(clampGaze(4, 9) == 4);
  assert(poseAt(0, 0).radius == 114);
  assert(poseAt(0, 0).lid == 1.0f);
  assert(std::fabs(poseAt(110, 0).lid - 0.1f) < 0.001f);
  assert(poseAt(300, 0).lid == 1.0f);
  assert(poseAt(1350, 1000).bob < poseAt(1350, 0).bob);
  assert(!poseAt(1699, 1000).bounceFinished);
  assert(poseAt(1700, 1000).bounceFinished);
  assert(poseAt(1000, 0, 1000).smile);
  assert(poseAt(1060, 0, 1000).scaleY < 1.0f && poseAt(1060, 0, 1000).scaleX > 1.0f);
  assert(poseAt(1190, 0, 1000).scaleY > 1.0f);
  assert(poseAt(1850, 0, 1000).scaleX == 1.0f);
  for (uint32_t a = 0; a < 850; ++a) {
    const auto p = poseAt(1000 + a, 0, 1000);
    assert(p.scaleX >= 0.85f && p.scaleX <= 1.15f);
    assert(p.scaleY >= 0.85f && p.scaleY <= 1.15f);
  }
  assert(poseAt(1849, 0, 1000).smile);
  assert(!poseAt(1849, 0, 1000).pokeFinished);
  assert(!poseAt(1850, 0, 1000).smile);
  assert(poseAt(1850, 0, 1000).pokeFinished);
  assert(poseAt(1000, 0, 0, 1000).dizzy);
  assert(poseAt(1300, 0, 0, 1000).swayX == 0);
  assert(poseAt(1300, 0, 0, 1000).spin > 3.0f);
  assert(!poseAt(1300, 0, 1000, 1000).smile);
  assert(poseAt(3499, 0, 0, 1000).dizzy && !poseAt(3499, 0, 0, 1000).dizzyFinished);
  assert(!poseAt(3500, 0, 0, 1000).dizzy && poseAt(3500, 0, 0, 1000).dizzyFinished);
  assert(poseAt(3500, 0, 0, 1000).swayX == 0);
  for (uint32_t a = 0; a < 2500; ++a) {
    const auto p = poseAt(1000 + a, 0, 0, 1000);
    assert(p.swayX >= -10 && p.swayX <= 10);
  }
  assert(poseAt(1500, 0, 0, 0, 1000).wave && poseAt(1500, 0, 0, 0, 1000).smile);
  assert(poseAt(1000, 0, 0, 0, 1000).armAngle == 0.0f);
  assert(poseAt(1300, 0, 0, 0, 1000).armAngle != poseAt(1350, 0, 0, 0, 1000).armAngle);
  assert(poseAt(2599, 0, 0, 0, 1000).wave && !poseAt(2599, 0, 0, 0, 1000).waveFinished);
  assert(!poseAt(2600, 0, 0, 0, 1000).wave && poseAt(2600, 0, 0, 0, 1000).waveFinished);
  assert(poseAt(2600, 0, 0, 0, 1000).armAngle == 0.0f);
  assert(!poseAt(1500, 0, 0, 1000, 1000).wave);
  for (uint32_t a = 0; a < 1600; ++a) {
    const float g = poseAt(1000 + a, 0, 0, 0, 1000).armAngle;
    assert(g >= -0.351f && g <= 0.351f);
  }
  assert(poseAt(1500, 0, 0, 0, 0, 1000).heart && poseAt(1500, 0, 0, 0, 0, 1000).smile);
  assert(poseAt(1000, 0, 0, 0, 0, 1000).heartScale == 0.0f);
  assert(poseAt(1300, 0, 0, 0, 0, 1000).heartScale > 0.8f);
  assert(!poseAt(1500, 0, 0, 0, 1000, 1000).wave);
  assert(poseAt(2999, 0, 0, 0, 0, 1000).heart && !poseAt(2999, 0, 0, 0, 0, 1000).heartFinished);
  assert(!poseAt(3000, 0, 0, 0, 0, 1000).heart && poseAt(3000, 0, 0, 0, 0, 1000).heartFinished);
  assert(poseAt(3000, 0, 0, 0, 0, 1000).heartScale == 0.0f);
  assert(!poseAt(1500, 0, 0, 1000, 0, 1000).heart);
  for (uint32_t a = 0; a < 2000; ++a) {
    const float g = poseAt(1000 + a, 0, 0, 0, 0, 1000).heartScale;
    assert(g >= 0.0f && g <= 1.3f);
  }
  assert(poseAt(1500, 0, 0, 0, 0, 0, 1000).mustache && poseAt(1500, 0, 0, 0, 0, 0, 1000).smile);
  assert(poseAt(1000, 0, 0, 0, 0, 0, 1000).mustacheScale == 0.0f);
  assert(poseAt(2000, 0, 0, 0, 0, 0, 1000).mustacheScale > 0.9f);
  assert(!poseAt(1500, 0, 0, 0, 0, 1000, 1000).heart);
  assert(!poseAt(1500, 0, 0, 0, 1000, 0, 1000).wave);
  assert(poseAt(3999, 0, 0, 0, 0, 0, 1000).mustache && !poseAt(3999, 0, 0, 0, 0, 0, 1000).mustacheFinished);
  assert(!poseAt(4000, 0, 0, 0, 0, 0, 1000).mustache && poseAt(4000, 0, 0, 0, 0, 0, 1000).mustacheFinished);
  assert(poseAt(4000, 0, 0, 0, 0, 0, 1000).mustacheScale == 0.0f);
  assert(!poseAt(1500, 0, 0, 1000, 0, 0, 1000).mustache);
  for (uint32_t a = 0; a < 3000; ++a) {
    const float g = poseAt(1000 + a, 0, 0, 0, 0, 0, 1000).mustacheScale;
    assert(g >= 0.0f && g <= 1.3f);
  }
  for (uint32_t t = 0; t < 100000; t += 37) {
    const auto p = poseAt(t, 0);
    assert(p.bob >= -3 && p.bob <= 3);
    assert(p.radius >= 112 && p.radius <= 116);
    assert(p.lid >= 0.1f && p.lid <= 1.0f);
    assert(!p.bounceFinished);
    assert(!p.mustache && !p.mustacheFinished && p.mustacheScale == 0.0f && !p.heart && !p.heartFinished && p.heartScale == 0.0f && !p.wave && !p.waveFinished && p.armAngle == 0.0f && !p.dizzy && !p.dizzyFinished && p.swayX == 0 && !p.smile && p.scaleX == 1.0f && p.scaleY == 1.0f && !p.pokeFinished);
  }
}
