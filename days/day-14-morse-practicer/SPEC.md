# Day 14: Morse Code Practicer behavior contract

## Objective

Make the left pusher a responsive straight key that teaches Morse timing by feel and sound.

## Shared platform contract

The target is the M5Stack Stopwatch with a corrected 468 × 466 drawable panel, rotation 0, x offset 6, and y offset 0.
BtnA is physical left and BtnB is physical right with the lanyard down.
Every touch consumer loads `espt-touch/record`, accepts safe version-1 and version-2 records, maps raw coordinates through the complete shared warp, and provides the shared runtime calibration flow.
No lesson-specific touch correction is permitted.

## Controls

Use a localized control scheme because A straight key must follow press duration directly. BtnA/left: key down up. BtnB/right: clear click speed hold. Short both: cycle speed. Hold both: clear message.
Touch and serial injection dispatch the same semantic action names.

## Portable state

- `wpm`: enum, default `10`
- `symbol`: string, default ``
- `message`: string, default ``
- `key_down`: bool, default `False`

State transitions are pure, deterministic, and leave their input value unchanged.
Every transition returns a state that satisfies the declared enum, integer, boolean, and string bounds.

## Behavior

1. BtnA starts a 600 Hz sidetone on press and stops it on release without blocking.
2. Use PARIS timing: unit_ms = 1200 / WPM; under two units is a dit and two units or more is a dah.
3. Decode a letter after a three-unit gap and a word after seven units. Invalid sequences show a question mark.
4. BtnB click clears; short both cycles 5, 10, 15, and 20 WPM; hold both clears and returns to 10 WPM.
5. Show the current symbol sequence and decoded text without clipping at every speed.

## Required scenarios

- **primary flow**: {'type': 'key', 'down': True}, {'type': 'key', 'down': False, 'duration_ms': 80}, {'type': 'gap', 'units': 3}
- **decode SOS**: {'type': 'decode', 'symbols': '...'}, {'type': 'decode', 'symbols': '---'}, {'type': 'decode', 'symbols': '...'}
- **speed cycle**: enter, enter, enter

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
