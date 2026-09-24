# Day 30: Trivia behavior contract

## Objective

Run a fair true/false quiz using identical M5Stack Stopwatches as host and buzzers.

## Shared platform contract

The target is the M5Stack Stopwatch with a corrected 468 × 466 drawable panel, rotation 0, x offset 6, and y offset 0.
BtnA is physical left and BtnB is physical right with the lanyard down.
Every touch consumer loads `espt-touch/record`, accepts safe version-1 and version-2 records, maps raw coordinates through the complete shared warp, and provides the shared runtime calibration flow.
No lesson-specific touch correction is permitted.

## Controls

Use a localized control scheme because True and False must be immediate one-button answers. BtnA/left: answer true. BtnB/right: answer false. Short both: host next. Hold both: leave game.
Touch and serial injection dispatch the same semantic action names.

## Portable state

- `role`: enum, default `choose`
- `screen`: enum, default `lobby`
- `qid`: int, default `0`
- `score`: int, default `0`
- `answer`: enum, default `none`

State transitions are pure, deterministic, and leave their input value unchanged.
Every transition returns a state that satisfies the declared enum, integer, boolean, and string bounds.

## Behavior

1. Choose Host or Player at startup. Hosts own the deck and qid; players answer True on BtnA and False on BtnB.
2. Timestamp the physical press locally, lock after the first answer, and reject duplicate or stale qids.
3. Use versioned ESP-NOW question, answer, verdict, score, and presence messages between identical Stopwatches.
4. Resolve ties by a documented clock-offset/round-trip method or declare them tied; never rank by packet arrival alone.
5. Run all twelve baked questions, keep scores consistent, and recover a player that briefly loses the host.

## Required scenarios

- **primary flow**: {'type': 'role', 'value': 'player'}, {'type': 'question', 'qid': 1}, left, {'type': 'verdict', 'correct': True}
- **first answer wins**: right, left
- **stale verdict ignored**: {'type': 'verdict', 'qid': 4, 'correct': True}

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
