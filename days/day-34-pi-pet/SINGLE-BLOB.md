# Day 34 standalone blob pet

This is a standalone pet app on the M5Stack Stopwatch. It keeps one Grok
Bot-style blue blob and two black idle eyes. A finger poke makes him smile with his eyes and
wobble like jelly. It has no Pi connection, roster, or host bridge. The earlier Pi Pet
experiment remains in the Day 34 history for design reference.

## Build prompt

```text
Build a disposable standalone one-blob pet for ESPtember Day 34 on the M5Stack
Stopwatch. Read AGENTS.md, the build-stopwatch-lessons and
prompt-first-embedded-lessons skills, the Grok Bot ESP32 skill, and the Day 07
touch contract first. Use Arduino ESP32 3.3.10, M5Unified 0.2.19, and M5GFX
0.2.26. Keep generated firmware and xAI-derived contour data in ignored .build/.

Immediately after M5.begin(), configure the panel to 468 x 466 with x offset
6, y offset 0, rotation 0. Use a black background. Center one #1084FE blob
in a 300 x 300 canvas at screen position (84,83), with a body radius of about
114 pixels. Derive the two 48-point idle eye contours from the bundled
.agents/skills/grok-bot-esp32/assets/eyes.json expression 0. Render them as black holes in
the body. Do not add another character, status band, page dots, or pairing UI.

Animate the blob's quiet idle motion and blink as specified below. A finger
tap inside the blob makes him smile with his eyes only (no mouth is drawn) for 850 ms and wobble like jelly, then return
to idle. Read raw touch and load a valid espt-touch/record after panel setup;
apply the complete version-2 warp when present. If no shared record exists,
use the M5GFX native conversion as an explicitly uncalibrated fallback. Offer
a serial-invoked guided runtime calibration that collects, observes, applies,
and saves a replacement record without reflashing. Reserve both physical
pushers for future navigation between pets. There is only one pet now; create
no speculative pets or timer. Flash only the active app partition found in the board's actual
partition table, preserving NVS and storage. Never use a merged image at 0x0
for this app update. Verify a booted status response and capture a device
framebuffer before describing the image as device evidence.
```

## Behavior contract

- The screen is 468 × 466, rotation 0. The 300 × 300 canvas starts at
  `(84,83)` and has a pure black background. No pixels are drawn below row 465.
- There is exactly one blue body. Its idle radius is 114 px, with a bounded
  `1.5 sin(1.9t)` px breathing variation. Its vertical bob is `3 sin(1.5t)` px.
- Every 4.2 s, the eyes blink over 220 ms. The eye contours compress toward
  their own horizontal centerlines and never close below 10% height.
- `look x y` moves both eyes, clamping x to `[-15,15]` and y to `[-9,9]` px.
  `reset` returns gaze to `(0,0)` and cancels a bounce.
- `bounce` starts a single 700 ms hop, with a maximum upward displacement of
  30 px and a decaying envelope. New commands may retrigger it.
- A new finger contact inside a 125 px radius of the current blob center
  triggers one poke. Both eyes become upward-curved crescents for 850 ms; no
  mouth is drawn. The body does a damped squash-then-stretch wobble: horizontal
  and vertical scale move in opposite directions, stay within 0.85-1.15, and
  settle to exactly 1.0 by 850 ms, after which the blob returns to idle. Holding a finger down does not retrigger it.
- A finger that lands on the blob grabs him: he follows the finger, keeping the
  grab offset, and is clamped so his body stays on the display. When the finger
  pushes him into any wall (left, right, top, or bottom) he gets dizzy: for 2.5 s both eyes are
  rotating spirals, with no body sway, and the smile and jelly wobble are
  suppressed. He can go dizzy again only after leaving the wall and hitting it
  again. On release he eases back to the center over 350 ms. `dizzy` triggers it over serial.
- Two quick taps on the blob (each under 300 ms with under 20 px of movement,
  the second within 450 ms of the first) make him wave for 1.6 s: happy
  crescent eyes and a round-tipped arm poking out to the upper right that
  swings about +/-0.35 rad. Dizziness cancels a wave. `wave` triggers it over
  serial.
- Three quick taps in a row (same tap rule, each within 450 ms of the last)
  show a red heart about 40 px wide above his head for 2.0 s, popping in with
  a small overshoot, pulsing, and shrinking away, with happy crescent eyes. It
  sits slightly on the top of his head. The third tap replaces the wave the
  second tap started. Dizziness cancels it. `heart` triggers it over serial.
- Four quick taps in a row (same tap rule) replace the heart with a black
  mustache under the eyes. It grows in over 0.5 s with a small overshoot, holds,
  and shrinks away in the last 0.3 s of a 3.0 s display, with happy crescent
  eyes. Dizziness cancels it. `mustache` triggers it over serial.
- `poke` triggers that same response over serial for injected-device checks.
  It is a diagnostic, not a second user interface.
- `status` reports `GB_STATUS blob=1`, frame count, display dimensions, and
  gaze. `shot` returns a `GB_SHOT 300 300 180000` header, exactly 180,000
  byte-swapped RGB565 bytes, then `GB_SHOT_END`.
- The left/A and right/B physical pushers are reserved for later navigation
  between pets. They do nothing while there is only one pet.
- Load `espt-touch/record` after panel geometry setup. Validate exact size,
  metadata, CRC, finite coefficients, determinant, and version-2 residual
  safety. Apply the full version-2 local residual warp to raw touch. A valid
  version-1 record uses its affine transform. With no valid record, use
  M5GFX's built-in coordinate conversion and report `map=0`. This is an
  uncalibrated fallback; it does not claim physical touch accuracy.
- `cal begin` shows center, up, down, left, and right targets in turn. Hold
  each dot steadily and release. Each accepted point needs 16 raw samples
  over at least 80 ms with no axis spanning more than 12 raw units. Raw
  medians and progress appear on serial. A least-squares affine fit is saved
  as a valid 440-byte version-2 record with zero residual field and an
  incremented generation, then applied immediately. `cal cancel` preserves
  the active map; `cal status` reports progress. This quick replacement flow
  does not reproduce Day 07's full residual-collection exercise.

## Acceptance

1. **Compile:** the pinned libraries and ESP32-S3 OPI PSRAM target compile an
   app image smaller than the active app partition. Validate its image hash.
   Run `tests/run-single-blob.sh PATH_TO_DISPOSABLE_CANDIDATE` to check the
   portable idle, blink, bounce, poke, gaze, and touch-map core without the board.
2. **Injected device:** flash only the active app slot; read `GB_STATUS`,
   exercise `poke`, `look`, `bounce`, and `reset`, and receive a complete `shot`
   frame. Inspect idle and smiling frames, the wobble across frames, shape,
   color, eye position, and black background.
3. **Physical review:** with the lanyard loop down, inspect centering,
   brightness, blinking, a real finger tap, smile, wobble, bottom edge, and
   unplugged operation. Confirm a tap near the center works and one outside
   the blob does not trigger a poke.
   Serial responses and a framebuffer capture do not establish these physical
   observations.

The source contours are derived from xAI's public Grok Bot design. Keep the
generated contour header and captured device data out of the committed lesson;
review rights before publishing derived assets or firmware publicly.
