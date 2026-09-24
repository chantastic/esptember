# Day 12: Basic-Ass Stopwatch behavior contract

## Objective

Use the device's physical form honestly: build a stopwatch with familiar pusher behavior.

## Shared platform contract

The target is the M5Stack Stopwatch with a corrected 468 × 466 drawable panel, rotation 0, x offset 6, and y offset 0.
BtnA is physical left and BtnB is physical right with the lanyard down.
Every touch consumer loads `espt-touch/record`, accepts safe version-1 and version-2 records, maps raw coordinates through the complete shared warp, and provides the shared runtime calibration flow.
No lesson-specific touch correction is permitted.

## Controls

Use a localized control scheme because Stopwatches have a stronger established convention than menu navigation. BtnA/left: start stop. BtnB/right: lap click reset hold. Short both: unused. Hold both: unused.
Touch and serial injection dispatch the same semantic action names.

## Portable state

- `mode`: enum, default `stopped`
- `elapsed_cs`: int, default `0`
- `laps`: int, default `0`
- `last_lap_cs`: int, default `0`

State transitions are pure, deterministic, and leave their input value unchanged.
Every transition returns a state that satisfies the declared enum, integer, boolean, and string bounds.

## Behavior

1. BtnA, physically left, always starts or stops. It never resets.
2. BtnB records a lap while running. While stopped, a BtnB click does nothing and a 600 ms hold resets.
3. Compute elapsed time from monotonic deltas and an accumulated base; rendering frequency must not change time.
4. Keep the newest three laps visible and retain more laps in the portable state up to its fixed capacity.
5. Format MM:SS.hh and handle monotonic-counter wrap without a backward jump.

## Required scenarios

- **primary flow**: left, {'type': 'tick', 'centiseconds': 123}, left
- **lap while running**: right
- **guarded reset**: right, right_hold

The JSON contract contains the exact intermediate expected state for each action.
It is normative when prose and a disposable candidate disagree.

## Visual contract

The reference frame is 468 × 466 and uses only rows 0–465.
The primary title, status, data, and control hint remain readable through a circular aperture.
Controls and their inset focus rings remain inside radius 226 from center `(234,233)`.
Touch targets are at least 42 pixels tall and lower controls remain unobstructed so the calibrated lower face is exercised.

## Persistence and failure contract

Only lesson-owned state may be written by this lesson.
The `espt-touch` namespace is read-only outside the shared calibration flow.
Validate persisted length, version, checksum where applicable, enum/range values, and string termination before use.
Network, sensor, audio, storage, allocation, and parse failures name their source on screen and over serial, retain the last safe state, and expose a bounded retry.

## Acceptance layers

1. The machine-readable contract passes three reducer shapes and rejects a deliberate mutation.
2. The selected portable implementation adds lesson-specific numerical, timing, parsing, and persistence tests.
3. The real libraries compile at pinned versions with no project warnings.
4. The instrumented device passes injected actions, capture, 20 largest-state loops, memory checks, and touch-map preservation.
5. A person reviews the physical controls, touch, cues, readability, clipping, bottom edge, and activity-specific behavior.

Each layer records only what it observes.
