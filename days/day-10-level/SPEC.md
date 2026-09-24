# Day 10: Level behavior contract

## Objective

Turn the Stopwatch into a bubble level that remains readable and stable while the board moves.

## Shared platform contract

The target is the M5Stack Stopwatch with a corrected 468 × 466 drawable panel, rotation 0, x offset 6, and y offset 0.
BtnA is physical left and BtnB is physical right with the lanyard down.
Every touch consumer loads `espt-touch/record`, accepts safe version-1 and version-2 records, maps raw coordinates through the complete shared warp, and provides the shared runtime calibration flow.
No lesson-specific touch correction is permitted.

## Controls

Use the default grammar: BtnA/left is Previous or Decrease; BtnB/right is Next or Increase; short both is Enter; hold both for 600 ms is Back. Preserve the 80 ms join and overlap rules and consume chord clicks.
Touch and serial injection dispatch the same semantic action names.

## Portable state

- `screen`: enum, default `bubble`
- `focus`: enum, default `zero`
- `zeroed`: bool, default `False`
- `level`: bool, default `False`
- `pitch`: int, default `0`
- `roll`: int, default `0`

State transitions are pure, deterministic, and leave their input value unchanged.
Every transition returns a state that satisfies the declared enum, integer, boolean, and string bounds.

## Behavior

1. Filter BMI270 pitch and roll without adding visible lag; define axis signs with the lanyard at the bottom.
2. Show a bubble, signed pitch and roll, and a LEVEL state within one degree, with hysteresis before leaving it.
3. Focus Zero and Mode. Enter on Zero stores the current attitude; Back clears the stored zero.
4. Support bubble and numeric modes without changing the sensor pipeline.
5. A transition into LEVEL emits one short cue; remaining level does not retrigger it.

## Required scenarios

- **primary flow**: enter, back
- **mode wrap**: right, enter, enter
- **level hysteresis**: {'type': 'sample', 'pitch': 0, 'roll': 0}, {'type': 'sample', 'pitch': 2, 'roll': 0}, {'type': 'sample', 'pitch': 4, 'roll': 0}

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
