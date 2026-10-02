#pragma once
#include <stdint.h>
struct RGB { float r, g, b; };
struct Rect { int x0, y0, x1, y1; };
// One eye as a capsule, in head units: centre, axis angle, half length, half width.
struct Cap { float cx, cy, ang, len, wid; };
struct Particle { float x, y, vx, vy, rot, vr, life, size; uint32_t color; };
enum class PetState : uint8_t { Idle, Thinking, Tool, Error, Done, Waiting, Sleeping };
struct Pet {
  bool used;
  char id[9], name[25], detail[41];
  PetState state;
  uint8_t style;
  uint32_t since;    // when `state` last changed
  bool unseen;       // finished/failed/asking and not yet acknowledged
  uint32_t errorAt;  // last Error report; the wince outlasts quick follow-up states
  bool uhohDone;     // the blocked 'uh-oh' already played for this error
};
