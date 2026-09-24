---
board: m5stack-stopwatch
day: 7
title: Touch Calibration
toolchain: Arduino CLI + esp32 3.3.10 + M5Unified 0.2.19 + M5GFX 0.2.26
summary: "Measure the touchscreen, learn its local distortion, and save a reusable map before LVGL."
verification: "Implementation prompt; hardware verification is part of the assignment"
---

## The assignment

Calibrate the M5Stack Stopwatch touchscreen before introducing LVGL.

This lesson stays below the widget layer.
It reads raw controller coordinates, learns the broad transform and the local distortion, and saves a map that later projects can update without rebuilding their firmware.

The durable source is the [behavior contract](https://github.com/chantastic/esptember/blob/main/days/day-07-touch-calibration/SPEC.md), the [testing model](https://github.com/chantastic/esptember/blob/main/days/day-07-touch-calibration/TESTING.md), and the prompt below.
The [three-build comparison](https://github.com/chantastic/esptember/blob/main/days/day-07-touch-calibration/RESULTS.md) records how independent implementations behaved against the same 1,052 assertions.

## The build prompt

Copy this prompt into your coding agent from the root of the ESPtember repository.

```text
Build ESPtember Day 07: Touch Calibration for the M5Stack Stopwatch Dev Kit (C152).

Read days/day-07-touch-calibration/SPEC.md before writing code.
Implement its touch_calibration_core.h API exactly and run
days/day-07-touch-calibration/tests/run-candidate.sh against the candidate.
Do not edit the contract tests to make an implementation pass.

The result must be a standalone Arduino CLI project for ESP32-S3 using:
- Arduino ESP32 core 3.3.10
- M5Unified 0.2.19
- M5GFX 0.2.26

Do not use LVGL in this lesson.
Draw directly with M5GFX so the touch layer is measured before a UI framework depends on it.

Hardware facts and orientation

- Target board: M5Stack Stopwatch Dev Kit, board_M5StopWatch.
- Hold it with the lanyard loop at the bottom.
- BtnA is physically left. BtnB is physically right.
- The installed libraries report 468 x 468, but the Stopwatch HAL uses a
  468 x 466 drawable area with offset_x = 6 and offset_y = 0.
- Immediately after M5.begin(), set panel width 468, height 466, offset_x 6,
  and offset_y 0. Then call setRotation(0).
- No screen may draw into rows 466 or 467. The result must have no black bar.
- Fail visibly and over serial if the board identity or touch controller is unavailable.

Interaction and button grammar

- Follow the C25K grammar: BtnA/left means Previous or Undo; BtnB/right means
  Next; a short two-button chord means Enter; holding both for 600 ms means
  Back or Cancel. A chord joins within 80 ms, overlaps for at least 80 ms,
  and never leaks individual button clicks.
- Touch collection advances automatically when it has enough data.
- Canceling a guided update returns to the application with the previously
  committed map unchanged.
- Holding both during boot for 600 ms enters the shared calibration exercise.

Guided stage 1: four cardinal drags

Use a controller-stick calibrator metaphor that suits the round face.

1. Draw a white center dot at (234,233) and one orange destination dot.
   Draw no line, ray, arrow shaft, or trail between the dots.
2. Show this exact instruction for the first step:
   "Drag from the center UP to the orange dot"
   Replace UP with DOWN, LEFT, and RIGHT for the following steps.
3. Use endpoints (234,63), (234,403), (64,233), and (404,233) in the order
   UP, DOWN, LEFT, RIGHT.
4. The orange destination dot is the only orange guidance mark in this stage.
   Turn it green after capture and keep completed destination dots visible.
5. Read calibration input with M5.Display.getTouchRaw(). Do not use already
   transformed M5.Touch x/y values as raw calibration data.
6. Before each direction, wait for release. Record the first raw contact near
   the center, but begin the endpoint stability window only after at least
   80 raw units of movement from that contact.
7. Collect at least 16 endpoint samples across at least 80 ms. Use the median
   raw x and y. Repeat only when either axis spans more than 12 raw units,
   the finger returns toward center, or it releases before enough samples.
8. Move a small white puck with the finger and explain retries in plain language.

Fit the affine seed

Map raw coordinates (rx, ry) to display coordinates (sx, sy):

sx = a*rx + b*ry + c
sy = d*rx + e*ry + f

Fit all four cardinal endpoint pairs with least squares.
Use a numerically stable dependency-free solver and reject singular or non-finite fits.
This seed handles the broad scale, offset, rotation, inversion, and skew.

Do not treat it as the complete calibration.
The observed Stopwatch error may expand or contract differently by radius and angle.

Guided stage 2: Around the World in both directions

Show a 170 px reference ring and collect two independent passes.

Pass 1 says "Trace the ring CLOCKWISE. Lift anytime."
Pass 2 says "Trace the ring COUNTERCLOCKWISE. Lift anytime."
Show a clear curved direction arrow and progress such as "CLOCKWISE 7/16".

For each pass:
- divide the face into 16 angular sectors,
- collect at least four finite raw samples in every sector,
- preserve progress across lifts,
- allow pauses, backtracking, and extra rotations,
- finish automatically after all sectors have enough data,
- and never reject a sample for being inside or outside the reference ring.

This is data collection, not a drawing test.
Do not display "stay closer," "stopped short," or any radial pass/fail message.
The direction label guides the exercise, but imperfect motion remains useful data.

For every sample, apply only the affine seed, preserve its observed angle, and
project that point onto the ideal 170 px ring.
The residual is projected target minus affine-mapped point.
Keep bounded per-sector aggregates outside the task stack.

Keep clockwise and counterclockwise measurements separate.
Calculate and report their vector difference in every sector as a diagnostic.
Then average the two per-sector residual vectors with equal weight, regardless
of how many raw samples either pass happened to contain.
Smooth the resulting circular ring with the contract's 1-2-1 kernel.
This becomes the outer residual ring at radius 170.

Guided stage 3: Asteroid Shooter

Present one target at a time and ask the user to tap its visible center.
Begin with a separate black screen containing only **ASTEROID SHOOTER** and
**Tap to start**. Discard that starting tap after release.

Then use exactly 51 attempts in a deterministic shuffled order:
- three targets centered at (234,233),
- one target at every 22.5-degree node on the 70 px ring,
- one target at every 22.5-degree node on the 120 px ring,
- and one target at every 22.5-degree node on the 170 px ring.

Vary the art without changing the measurement:
- filled circle, outlined ring, hexagonal rock, and irregular asteroid styles,
- visual radii 10, 14, 18, and 22 px,
- orange, cyan, lime, magenta, and white,
- and a 2 px white center pip on every target.

The target center is always the calibration truth, independent of its style,
size, and color.
The shuffle must distribute styles across radii and angles and must be identical
for every conforming build so injected runs are reproducible.

During the 51 attempts, the screen contains only the current target on black.
Do not show a title, instructions, progress, attempt count, footer, button hint,
previous target, impact marker, miss vector, toast, or result.
Advance directly to the next target after release.
This leaves the lower and outer display available for calibration targets.

Accept every finite touch release, whether it hits or misses the drawing.
A quick tap with one finite raw sample is valid; for a contact containing several
samples, use the component-wise median.
Do not grade, reject, or ask the user to retry because the tap missed.

Record target id, target center, raw median, affine-mapped point, residual vector,
and miss distance.
BtnA undoes the most recent attempt before the next target is accepted.

Use the component-wise median of the three center residuals as the center value.
Use each 70 px, 120 px, and 170 px target residual at its matching angular node.
Smooth all three rings with the same circular 1-2-1 kernel.
Combine the smoothed 170 px target ring with the equal-direction-weight perimeter
ring using equal weight, then smooth that combined outer ring once more.

Build the local warp map

The final map contains:
- the six-coefficient affine seed,
- one center residual,
- 16 residual vectors at radius 70,
- 16 residual vectors at radius 120,
- and 16 equal-direction-weight residual vectors at radius 170.

Apply the affine transform first.
Then interpolate the residual field in polar display space: wrap angularly from
sector 15 to sector 0; interpolate radially from center through 70, 120, and
170 px; and clamp the residual outside radius 170.
Add the interpolated residual to the affine point.

Validate the field with the magnitude and neighboring-delta limits in SPEC.md.
If it is unsafe, identify the offending local targets or sectors and collect
only those measurements again.
Do not restart the entire exercise and do not blame the user's trace.

Persistence and live updates

Store one RecordV2 Preferences blob named "record" in namespace "espt-touch".
It is exactly 440 bytes and includes version 2 metadata, generation, affine seed,
center residual, all three 16-sector rings, and the specified CRC-32.

At boot, validate byte count, version, display geometry, rotation, fixed ring
counts, checksum, affine transform, finite residuals, and safety limits.
Increment generation each time a new map commits.

Recognize the older 40-byte version-1 record.
Load it as an affine transform with zero local residuals, keep it usable, and
offer the guided exercise to upgrade it to version 2.

Calibration is device-owned runtime state.
Never generate source containing one device's coefficients.
Commit the new blob only after the full candidate map validates; apply it in RAM
immediately; and preserve the old blob when the user cancels.

M5GFX can apply the affine seed, but it cannot apply this local residual field.
The reusable module must therefore read raw touch and call applyWarp() for every
application touch.
Later LVGL pointer callbacks consume the module's mapped point and contain no
additional offsets, axis swaps, scale factors, or calibration constants.

An app-only or OTA update preserves the Preferences namespace.
A merged fresh-install image written at address 0x0 may erase it and must be
labeled as resetting calibration.

Result screen and real-time observation

Show a readable summary containing:
- CALIBRATED or NEEDS CALIBRATION,
- saved map generation,
- clockwise/counterclockwise difference,
- Asteroid Shooter mean and maximum miss distance before correction,
- affine coefficients,
- a before/after vector-field preview,
- "Both: run guided update",
- and "Hold both: back".

Stream throttled observations throughout collection with phase, target or sector,
raw x/y, affine x/y, final mapped x/y, residual x/y, and progress.
Logging and display updates must consume the same observations without changing
sampling, aggregation, fitting, or timing.

Add serial commands:
- status: report active version, generation, validity, and last diagnostics,
- calibrate: enter the complete three-stage guided update,
- observe on|off: enable or disable live observation lines,
- export: print the active map in machine-readable form,
- erase: clear only espt-touch/record and enter calibration.

Checks

- Host tests cover the affine solver, bidirectional perimeter coverage, equal
  direction weighting, varied-radius traces, polar interpolation, angular seam,
  asymmetric local scale, smoothing, safety limits, version-2 checksum and
  tamper rejection, and version-1 migration.
- Add an instrumented mode that proves the deterministic 51-target schedule,
  accepts deliberate misses, supports Undo, builds all four residual layers,
  and commits only a complete valid map.
- Build with no warnings introduced by this project.
- Flash the attached Stopwatch and complete all three stages without a reboot,
  blank screen, clipped instruction, black bar, or stack-canary failure.
- Confirm each cardinal screen contains one orange destination dot and no orange
  connecting line, and uses the exact instruction sentence.
- Confirm both Around the World passes retain progress across lifts and accept
  visibly imperfect radii.
- Confirm the shooter has a tap-to-start screen; every active test frame contains
  only one target on black; lower-ring targets are unobstructed; and deliberate
  misses appear in the map data.
- Reboot and prove the version-2 map loads without reopening calibration.
- Run a second guided update without reflashing and prove mapped touch changes
  immediately and survives another reboot.
- Install an app-only test build and prove the record survives.
- Erase calibration and prove only the touch record is removed.

Document exact observed results.
Never claim physical accuracy from injected touches, host tests, or compilation.
```

## Why this comes before LVGL

LVGL should receive screen coordinates.
It should not need to know how a particular touch controller stretches, shifts, or bends its input across the face.

The four cardinal drags establish the broad transform.
The two perimeter traces reveal direction-dependent edge behavior.
Asteroid Shooter measures the interior without asking the user to trace an ideal path.

Together they produce a map that can describe the asymmetric, zoom-like error seen on the physical device.

## Recorded evidence

Three disposable portable-core implementations pass the same 1,052 assertions.
Those tests cover the affine seed, the deterministic 51-target schedule, quick and imperfect tap reduction, perimeter collection in both directions, equal direction weighting, polar residual interpolation, smooth angular wrap, local asymmetric scale, persistence, corruption, and version-1 migration.

A deliberate threshold mutation still fails two assertions and returns a nonzero status.

The first September 23, 2026 hardware run predates the local warp exercise.
It completed the four cardinal drags, collected 464 perimeter samples, and exposed a signed deviation from 20.86 px inward to 16.53 px outward.
Its single affine refinement reached 8.05 px mean and 16.20 px maximum residual and survived reboot as a valid version-1 record.

An interim version-2 run then completed both perimeter directions and the original 35-target shooter.
The clockwise pass collected 352 samples with 7.54 px mean radial deviation; the counterclockwise pass collected 389 with 9.75 px mean deviation.
Their per-sector difference averaged 8.67 px and reached 27.38 px, confirming that direction is useful diagnostic data.
The shooter recorded 12.49 px mean and 25.03 px maximum miss distance, saved a 440-byte generation-1 map, and reached PASS.

That run exposed the next prompt issue: shooter labels occupied the part of the display that most needed measurement.

The revised 51-target run used a separate Tap to start screen and showed only one target on black during collection.
It reached the new 170-pixel ring, recorded 15.15 px mean and 29.57 px maximum affine miss distance, saved generation 2, and reloaded that version-2 map successfully.
The maximum occurred at the bottom-center target `(234,403)`, where the residual was approximately `(0.3,-29.6)` pixels.
That measurement confirms why the lower face needed to remain unobstructed.

## What we learned

- A calibration gesture gathers evidence; it does not grade the person holding the device.
- Opposite trace directions should remain separate long enough to reveal drag and hysteresis.
- Targets across multiple radii reveal local scale error that one affine transform cannot represent.
- Misses are calibration data when the intended target is known.
- Saved calibration belongs to the device and can change at runtime.

The next lesson can teach widgets after this one teaches the glass where the finger landed.
