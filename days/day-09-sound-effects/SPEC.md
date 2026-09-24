# Day 09: Sound Effects Board behavior contract

## Objective

Build a fast, two-page drum-pad instrument for the M5Stack Stopwatch.
Each page shows four colored square pads in a 2 × 2 grid.
The left and right pushers change pages; touching a pad triggers its sound immediately.

The board has eight configurable slots arranged as two coherent game-sound kits.
The Mario page contains Jump, Coin, Death, and Power Up.
The Asteroid page contains Shoot, Explode, Crash, and Power Up.
The two Power Up slots have separate recipes appropriate to their page.

## Shared platform contract

The target is the M5Stack Stopwatch with a corrected 468 × 466 drawable panel, rotation 0, x offset 6, and y offset 0.
BtnA is physical left and BtnB is physical right with the lanyard down.
Every touch consumer loads `espt-touch/record`, accepts safe version-1 and version-2 records, maps raw coordinates through the complete shared warp, and provides the shared runtime calibration flow.
No lesson-specific touch correction is permitted.

## Localized controls

This lesson intentionally uses a drum-machine control scheme instead of the C25K focus grammar.

- BtnA/left moves to the previous page and wraps.
- BtnB/right moves to the next page and wraps.
- With two pages, either pusher toggles the visible page in its documented direction.
- A pad triggers on touch-down through `LV_EVENT_PRESSED`; it does not wait for release or require a two-button chord.
- Holding both pushers for 600 ms during boot enters the shared touch-calibration flow.
- Simultaneous runtime pusher presses do not trigger pads or change two pages at once.

The footer shows a left arrow beside A, the page count, and a right arrow beside B.

## Portable state

- `page`: enum, default `page1`
- `active`: enum, default `none`
- `last_trigger`: enum, default `none`
- `plays`: integer, default `0`, range 0–9999

State transitions are pure, deterministic, and leave their input value unchanged.
Every transition returns a state that satisfies the declared bounds.

`pad1_down` through `pad4_down` resolve through the current page's slot manifest.
An empty slot leaves audio state and play count unchanged.

## Visual contract

Each page contains four 132 × 118 colored pads arranged at `(84,88)`, `(252,88)`, `(84,246)`, and `(252,246)`.
The pad is the color field; its name appears in small text immediately underneath.
Do not put large words, icons, focus rings, counters, or instructions inside a pad.

The page title stays above the grid and names the active kit: `MARIO` or `ASTEROID`.
The footer shows `A ◀`, `page / count`, and `▶ B`.
All pad corners and text remain inside the radius-226 safe area around `(234,233)`.
The screen uses rows 0–465 only and leaves no bottom strip.

Use distinct colors with sufficient luminance contrast on the black background.
The Mario defaults are orange, cyan, purple, and green.
The Asteroid defaults are cyan, orange, red, and blue.
All eight default slots are configured, so both pages remain complete 2 × 2 instruments.

## Sound configuration

The generated project owns a checked-in or generated `sound_manifest` with at most eight ordered slots.
Each slot declares a short label, pad color, and exactly one source:

1. **Description:** natural-language intent plus an explicit deterministic recipe produced from it.
2. **File:** a user-supplied WAV, AIFF, MP3, FLAC, or OGG file plus the reproducible conversion command and source filename.

A described sound is translated before compilation into a bounded recipe containing:

- oscillator or noise type;
- start and end frequency where applicable;
- duration in milliseconds;
- attack, decay, sustain, and release or a named exponential envelope;
- optional low-pass coefficient, tremolo, vibrato, or seeded noise; and
- output gain.

Show the recipe beside the description so a user can edit concrete values after hearing it.
Seed every noise source explicitly.
The same recipe must produce the same signed 16-bit PCM and hash on every build.

A supplied file is converted before compilation to signed 16-bit little-endian, mono, 22,050 Hz PCM.
Preserve the original file outside generated build output.
Reject unreadable input, clips longer than two seconds, decoded data larger than the configured PSRAM budget, and a silent result.
Do not decode MP3, FLAC, OGG, or resample audio inside a touch callback.

Empty slots remain valid for a customized manifest, but the default manifest has none.
Changing labels, colors, descriptions, or files regenerates a disposable candidate; it does not change the hardware or interaction contract.

## Fast trigger path

Prepare every configured clip before the UI becomes interactive.
Render descriptions into PSRAM at startup and decode or convert supplied files at build time or startup outside the UI task.
Keep the final PCM buffers resident and immutable while the app is running.

On `LV_EVENT_PRESSED`:

1. resolve the current page and pad index;
2. update `active`, `last_trigger`, and `plays`;
3. stop/retrigger speaker channel 0 with the resident PCM buffer; and
4. return without synthesis, file I/O, allocation, logging loops, animation waits, or release detection.

There is one voice.
A new pad immediately replaces the sound currently playing; there is no pending queue and no wait for the old clip to finish.
Repeated presses of the same pad retrigger from its first sample.

Measure latency from mapped touch-down or pusher edge to the corresponding state transition and from pad touch-down to `D09_AUDIO started`.
The page-state transition must occur within 16 ms, and pad-to-audio-start must be at most 30 ms at the 95th percentile over 100 injected triggers.
No single measured trigger may exceed 50 ms.

## Required scenarios

- **primary flow:** trigger Mario Jump, then complete playback;
- **immediate retrigger:** trigger Mario Coin and Power Up while another clip is active; each replaces it immediately;
- **pusher paging:** right, left, then wrapping left;
- **Asteroid page:** trigger Shoot, Explode, Crash, and Power Up through page 2;
- **kit identity:** the visible page title and every pad label match the active page's four-slot manifest.

The JSON contract contains the exact intermediate expected state for every action.
It is normative when prose and a disposable candidate disagree.

## Diagnostics and failure behavior

Prefix serial lines with `D09_` and provide `status`, `reset`, `action`, `capture`, `touchlog`, and `calibrate` commands.
Report board identity, geometry, calibration version and generation, page, active clip, last trigger, play count, free memory, stack headroom, and last named error.

For every trigger report slot, source type, sample count, PCM hash, request timestamp, start timestamp, and measured latency.
Do not print PCM or perform unbounded serial writes on the trigger path.

An invalid description recipe, missing file, decode failure, allocation failure, or speaker failure disables only that slot when possible, labels it `ERROR`, names the source on screen and serial, and keeps other pads usable.
The `espt-touch` namespace is read-only outside the shared calibration flow.

## Acceptance layers

1. The machine-readable contract passes three reducer shapes and rejects a deliberate mutation.
2. Domain tests verify every recipe or converted file's length, bounds, deterministic hash, non-silence, and PSRAM budget.
3. The real libraries compile at pinned versions with no project warnings.
4. The instrumented device passes 100-trigger latency measurement, immediate retriggering, injected touch hit-testing, page buttons, framebuffer capture, 20 page round trips, memory checks, and touch-map preservation.
5. A person reviews pad responsiveness, every configured sound, pusher direction, touch alignment, readability, clipping, and the bottom edge.

Each layer records only what it observes.
