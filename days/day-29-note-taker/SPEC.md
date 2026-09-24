# Day 29: Note Taker behavior contract

## Objective

Record a short memo, transcribe it, and keep the result locally browsable even when the network later disappears.

## Shared platform contract

The target is the M5Stack Stopwatch with a corrected 468 × 466 drawable panel, rotation 0, x offset 6, and y offset 0.
BtnA is physical left and BtnB is physical right with the lanyard down.
Every touch consumer loads `espt-touch/record`, accepts safe version-1 and version-2 records, maps raw coordinates through the complete shared warp, and provides the shared runtime calibration flow.
No lesson-specific touch correction is permitted.

## Controls

Use a localized control scheme because Push-to-record must follow a held pusher; browsing remains one-handed. BtnA/left: record hold. BtnB/right: browse next. Short both: open selected. Hold both: return idle.
Touch and serial injection dispatch the same semantic action names.

## Portable state

- `screen`: enum, default `idle`
- `notes`: int, default `0`
- `selected`: int, default `0`
- `duration_ms`: int, default `0`
- `text`: string, default ``

State transitions are pure, deterministic, and leave their input value unchanged.
Every transition returns a state that satisfies the declared enum, integer, boolean, and string bounds.

## Behavior

1. BtnA records only while held, discards presses under 500 ms, and stops at 30 seconds without overflow.
2. Show Recording, Transcribing, Saving, Saved, and Failed as distinct states with elapsed time or a named reason.
3. Store at most 20 notes in a crash-safe ring with timestamp and text; BtnB browses newest first.
4. Never store or log the transcription key. Keep PCM in PSRAM and free it on every success or failure path.
5. A failed upload retains no phantom note and can retry without recording again while PCM remains available.

## Required scenarios

- **primary flow**: {'type': 'record', 'down': True}, {'type': 'record', 'down': False, 'duration_ms': 2200}, {'type': 'transcript', 'text': 'Buy coffee.'}
- **discard pocket noise**: {'type': 'record', 'down': True}, {'type': 'record', 'down': False, 'duration_ms': 300}
- **browse**: right, right

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
