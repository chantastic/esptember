# Day 32: Connected behavior contract

## Objective

Turn one approved device flow into a durable, renewable, visible session.

## Shared platform contract

The target is the M5Stack Stopwatch with a corrected 468 × 466 drawable panel, rotation 0, x offset 6, and y offset 0.
BtnA is physical left and BtnB is physical right with the lanyard down.
Every touch consumer loads `espt-touch/record`, accepts safe version-1 and version-2 records, maps raw coordinates through the complete shared warp, and provides the shared runtime calibration flow.
No lesson-specific touch correction is permitted.

## Controls

Use the default grammar: BtnA/left is Previous or Decrease; BtnB/right is Next or Increase; short both is Enter; hold both for 600 ms is Back. Preserve the 80 ms join and overlap rules and consume chord clicks.
Touch and serial injection dispatch the same semantic action names.

## Portable state

- `screen`: enum, default `boot`
- `focus`: enum, default `card`
- `expires_s`: int, default `0`
- `refreshes`: int, default `0`
- `resident`: bool, default `False`

State transitions are pure, deterministic, and leave their input value unchanged.
Every transition returns a state that satisfies the declared enum, integer, boolean, and string bounds.

## Behavior

1. Persist the refresh token atomically and replace it immediately whenever the provider rotates it.
2. Attempt silent refresh on boot before starting a new device flow. Never display or log either token.
3. Show safe claims, organization, role, session id suffix, and a monotonic expiry countdown.
4. Refresh before expiry with bounded backoff. Keep the current session until invalid_grant proves it unusable.
5. Navigate Card, Claims, and Sign Out; require confirmation before deleting residency.

## Required scenarios

- **primary flow**: {'type': 'refresh_ok', 'expires_s': 300}, {'type': 'second'}
- **claims**: right, enter, back
- **guarded signout**: right, enter, back

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
