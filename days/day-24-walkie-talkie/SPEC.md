# Day 24: Walkie-Talkie behavior contract

## Objective

Use two M5Stack Stopwatches as a local voice link with one familiar push-to-talk control.

## Shared platform contract

The target is the M5Stack Stopwatch with a corrected 468 × 466 drawable panel, rotation 0, x offset 6, and y offset 0.
BtnA is physical left and BtnB is physical right with the lanyard down.
Every touch consumer loads `espt-touch/record`, accepts safe version-1 and version-2 records, maps raw coordinates through the complete shared warp, and provides the shared runtime calibration flow.
No lesson-specific touch correction is permitted.

## Controls

Use a localized control scheme because Push-to-talk must track a held physical pusher directly. BtnA/left: ptt hold. BtnB/right: next channel. Short both: channel select. Hold both: return idle.
Touch and serial injection dispatch the same semantic action names.

## Portable state

- `mode`: enum, default `idle`
- `channel`: int, default `1`
- `sequence`: int, default `0`
- `queued`: int, default `0`
- `lost`: int, default `0`

State transitions are pure, deterministic, and leave their input value unchanged.
Every transition returns a state that satisfies the declared enum, integer, boolean, and string bounds.

## Behavior

1. BtnA transmits only while held. BtnB advances channels 1–22; short both opens direct channel selection.
2. Capture 8 kHz, 16-bit mono in fixed frames that remain under the ESP-NOW payload limit.
3. Version packets, sequence them, reject other channels, and count loss, late, duplicate, and malformed frames.
4. Use a bounded jitter buffer with a named prefill target and silence insertion for missing frames.
5. Never play local microphone audio and never block UI/button/touch work during capture or playback.

## Required scenarios

- **primary flow**: {'type': 'ptt', 'down': True}, {'type': 'frame_sent'}, {'type': 'ptt', 'down': False}
- **channel wrap**: right
- **receive jitter**: {'type': 'frame_received', 'sequence': 10}, {'type': 'frame_received', 'sequence': 12}

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
