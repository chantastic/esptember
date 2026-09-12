# C25K for M5Stack StopWatch — Design Spec

Version 1.1 · 2026-09-12

This is the project's source of truth for product behavior, the workout program, and saved data.
Change this document alongside any agreed behavior or storage change, then keep the firmware, tests, and guides consistent with it.
An implementation mismatch belongs in §14 until resolved; it does not silently redefine the requirement.

The [original version 1.0](spec-history/v1.0.md) is preserved unchanged.
[BUILD.md](BUILD.md) covers operating and building the device, [README.md](README.md) is the published guide, and [STORY.md](STORY.md) records how it was built.
Those documents support this plan.
The document revision is separate from the firmware version shown in About.

## 1. Purpose

A Couch-to-5K interval trainer that runs natively on the M5Stack StopWatch (C152). Button-only operation, designed to be used mid-run without looking at the screen for anything but a glance. Tracks active session time and time spent running — no distance, no GPS, no HR.

This is an isolated, offline application: the workout program ships with the firmware and progress lives on the device.
It has no account, backend, or cloud copy of the log.

## 2. Hardware (as-shipped, C152)

| Component | Detail | Used for |
|---|---|---|
| MCU | ESP32-S3R8, 16MB Flash, 8MB PSRAM | — |
| Display | 1.75" round AMOLED, 466×466, CO5300 over QSPI | All UI |
| Touch | CST820B | **Not used** (running use case) |
| Buttons | KEYA = Yellow (G2), KEYB = Blue (G1), + power button | Primary input |
| Haptic | Built-in vibration motor (via M5IOE1 PYG9 PWM) | Segment cues |
| Audio | ES8311 codec, AW8737A amp, 1W speaker | Segment cues |
| RTC | RX8130CE | Log timestamps |
| Power | M5PM1, 450mAh battery | — |
| IMU | BMI270 | Unused in v1 |

M5Unified mapping: `M5.BtnA` = Yellow, `M5.BtnB` = Blue. Power button is handled by M5PM1 and left alone.

The panel's nominal resolution is 466×466; the verified M5Unified/M5GFX driver exposes a 468×468 drawing surface.
Use the driver's dimensions and keep the layout inside the circular aperture.
Orientation is handheld, with the lanyard loop at the bottom.

## 3. Input grammar

Four inputs, globally consistent. Every screen uses the same four and only these four.

| Input | Name | Detection |
|---|---|---|
| Blue press | **Next** | BtnB `wasClicked()` with BtnA not held |
| Yellow press | **Prev** | BtnA `wasClicked()` with BtnB not held |
| Both pressed together | **Enter** | Presses join within 80 ms, overlap for at least 80 ms but less than 600 ms, then both release |
| Both held together | **Escape** | Presses join within 80 ms and both remain held for at least 600 ms |

Rules:

- Once a "both" gesture is detected, the individual clicks that made it up are swallowed — they never fire as Next/Prev.
- Hold-both and press-both are exclusive: if the hold threshold is crossed, Enter does not fire on release.
- Enter waits for both buttons to release. Escape fires once at the hold threshold; release both before another gesture.
- A late second press cannot upgrade a single-button gesture into Enter or Escape. An overlap shorter than 80 ms can produce only the first button's valid click, after both release.
- No single-button holds are used anywhere. Keeps the grammar to exactly four.
- Touch input is ignored entirely.

### 3.1 Track-back behavior (in-workout Prev)

Media-player convention:

- Prev press → restart the **current** segment.
- Prev press again within **2 s** of the previous Prev → jump to the **previous** segment.
- Chain continues: each subsequent Prev within 2 s goes back one more segment.
- Next, pause/resume, and opening or dismissing Cancel Confirm end the Prev chain.
- At segment 0 (warm-up), the first Prev restarts warm-up; further chained presses cannot go earlier and are no-ops.

### 3.2 Track-forward behavior (in-workout Next)

- Next → advance immediately to the start of the next segment.
- Next on the final segment → ends the workout (goes to Complete screen, flagged partial — see §7).

## 4. Screen architecture

Home, History, and Settings use horizontal paging, one item per page, with dot indicators along the bottom arc and the current dot filled.
Confirmation and result screens show their available actions explicitly.

| From | Action | To |
|---|---|---|
| Home, workout page | Enter | Workout |
| Home, History page | Enter | History |
| Home | Escape | Settings |
| Settings or History | Escape | Home at the cursor |
| Workout | Final segment ends, or Next on final segment | Complete |
| Workout | Escape | Cancel Confirm |
| Cancel Confirm | Escape again | Home, session discarded |
| Cancel Confirm | Other input or five-second timeout | Workout |
| Complete | Enter or Escape | Home at the new cursor |
| Settings, Reset Progress | Enter | Reset Confirm |
| Reset Confirm | Enter | Reset Result: success or failure |
| Reset Confirm | Escape | Settings, without resetting |
| Reset Result, success | Enter or Escape | Home at W1 D1 |
| Reset Result, failure | Enter / Escape | Retry reset / return to Settings |

Top-level toggle: **Escape on Home → Settings. Escape on Settings → Home.**
History is reached by paging past the last workout on Home (see §4.1), and Escape from History returns to Home.

### 4.1 Home

- Horizontal filmstrip of the 27 workouts, W1D1 → W9D3, then one final page: **History**.
- Blue/Yellow page right/left. Wraps at both ends.
- Each workout page shows, within the circular safe area:
  - Large label: `W3 D2`
  - Total session duration, including warm-up and cool-down (e.g. `28:00 total`).
  - An activity summary before starting: RUN/WALK durations in their exact order, read left to right and then top to bottom.
  - A round count when the complete core is an exact repeated sequence; show the shortest exact sequence once. Otherwise show every interval. A single run is labeled `CONTINUOUS RUN`.
  - Warm-up and cool-down durations separately, below the core sequence.
  - Full and partial completion counts for this workout and the date of the newest saved session (blank if never run). Partial counts use `~`. Counts cover the retained log, not lifetime totals.
  - `BOTH TO START`.
- The activity summary is derived from the same segment table as the timer; it must reconstruct every core segment exactly. Do not maintain separate handwritten workout descriptions.
- Example: W1 D1 shows `8 ROUNDS`, `RUN 1:00 → WALK 1:30`, `5:00 WARM UP + 5:00 COOL DOWN`, and `30:00 total`. W4 shows all seven core intervals without shortening a partial repeat.
- The **cursor** (next-up workout) is marked with a small filled triangle under its page label and its bottom-arc dot is drawn larger/brighter. The cursor is *not* the same as the page you're currently viewing.
- On boot, Home opens on the cursor page.
- **Enter** → start whatever workout is on the current page.
- **Escape** → Settings.

### 4.2 Workout (active)

Layout, top to bottom, all centered:

- Thin **session-progress arc** around the outer edge: position in the planned sequence, including warm-up/cool-down. It moves back on replay and forward on skip; actual elapsed time is accounted separately (§10).
- Thicker **segment-progress ring** just inside it, fills clockwise as the current segment elapses.
- Segment label: `WARM UP` / `RUN` / `WALK` / `COOL DOWN`.
- Segment countdown, largest text on screen: `M:SS`.
- Below: `Seg 5/13` and elapsed session time in small text.
- Bottom arc: dots = one per segment, current dot filled. Run segments drawn in the run color, walk segments in the walk color, so the pattern is readable at a glance.

Controls:

| Input | Action |
|---|---|
| Blue | Next segment |
| Yellow | Track-back (§3.1) |
| Enter | Pause / Resume |
| Escape | Open Cancel Confirm |

Paused state: countdown dims, `PAUSED` replaces the segment label, ring stops. Any Enter resumes. Blue/Yellow still work while paused (they act, then remain paused).

Screen: **always on, full brightness**, including while paused. The current build also keeps full brightness on other pages. Power-saving deferred to v2.

### 4.3 Cancel Confirm

Overlay on the workout screen (workout keeps timing underneath until confirmed — no auto-pause):

- Text: `Cancel workout?` / `Hold both again to confirm`
- **Escape** → confirm cancel → discard session, nothing logged, return to Home with cursor unchanged.
- **Any other input** (Blue, Yellow, Enter) → dismiss overlay, continue workout.
- Overlay auto-dismisses after 5 s of no input.
- Natural completion still opens Complete if the workout finishes while this overlay is showing.

### 4.4 Complete

Reached automatically when the final segment's countdown hits zero, or by pressing Next on the final segment.

- Shows `DONE` (or `DONE ~` if partial), workout label, total time, run-time total.
- The log save is attempted here (§7). A failed save shows `LOG COULD NOT BE SAVED`; reaching Complete alone does not prove persistence succeeded.
- Cursor advances here (§6).
- Enter or Escape → Home (opens on the new cursor page).
- Haptic: three short buzzes. Audio: short ascending three-note pattern.

### 4.5 History

- Filmstrip, one log entry per page, newest first.
- Page shows: workout label, date + time (from RTC), total duration, run-time total, `PARTIAL` badge if flagged.
- Blue/Yellow page; wraps.
- Long histories use a moving window of dots and an explicit entry number rather than squeezing 256 dots onto the arc.
- Empty state: `No runs yet`.
- **Escape** → Home.
- Enter does nothing (no per-entry actions in v1).

### 4.6 Settings

Filmstrip, one setting per page:

1. **Sound** — On / Off. Enter toggles.
2. **Vibration** — On / Off. Enter toggles.
3. **Set Time** — Enter opens an hour → minute → confirm sub-flow using Blue/Yellow to adjust, Enter to advance, Escape to back out. Preserve the calendar date; date and timezone limitations are in §7.3.
4. **Reset Progress** — Enter opens the confirmation and result flow below. Not needed for repeats (§6) — this is for starting the whole program over.
5. **About** — firmware version, build date, battery %.

**Escape** on any settings page → Home.

#### Reset confirmation and result

- Confirmation shows `Start over?`, how many saved sessions will be cleared, and `Return to W1 D1`.
- The confirming instruction is explicit: `PRESS BOTH + RELEASE` / `TO RESET`. **Holding both cancels** and returns to Settings with progress unchanged.
- Reset clears history, the cursor, and the program-complete flag. It preserves Sound, Vibration, and the RTC clock.
- Build a cleared candidate, save it, and verify readback before replacing the current in-memory progress (§7.4).
- Success shows `History cleared` and `W1 D1 is next`. Enter or Escape returns Home at W1 D1.
- Failure shows `Reset not confirmed` with a retry action. Keep the current in-memory history. Enter retries; Escape returns to Settings.
- A failed readback does not prove flash was unchanged: do not report success or promise flash rollback. The reset flow must distinguish success, failure, and cancellation.

## 5. Workout program

Standard 9-week Couch-to-5K plan. Every session begins with a 5:00 warm-up walk and ends with a 5:00 cool-down walk; the table below lists only the middle. Times in minutes:seconds.

| Wk | Day | Segments (R = run, W = walk) | Core | Total |
|---|---|---|---|---|
| 1 | 1–3 | (R 1:00, W 1:30) × 8 | 20:00 | 30:00 |
| 2 | 1–3 | (R 1:30, W 2:00) × 6 | 21:00 | 31:00 |
| 3 | 1–3 | (R 1:30, W 1:30, R 3:00, W 3:00) × 2 | 18:00 | 28:00 |
| 4 | 1–3 | R 3:00, W 1:30, R 5:00, W 2:30, R 3:00, W 1:30, R 5:00 | 21:30 | 31:30 |
| 5 | 1 | R 5:00, W 3:00, R 5:00, W 3:00, R 5:00 | 21:00 | 31:00 |
| 5 | 2 | R 8:00, W 5:00, R 8:00 | 21:00 | 31:00 |
| 5 | 3 | R 20:00 | 20:00 | 30:00 |
| 6 | 1 | R 5:00, W 3:00, R 8:00, W 3:00, R 5:00 | 24:00 | 34:00 |
| 6 | 2 | R 10:00, W 3:00, R 10:00 | 23:00 | 33:00 |
| 6 | 3 | R 25:00 | 25:00 | 35:00 |
| 7 | 1–3 | R 25:00 | 25:00 | 35:00 |
| 8 | 1–3 | R 28:00 | 28:00 | 38:00 |
| 9 | 1–3 | R 30:00 | 30:00 | 40:00 |

Stored in firmware as 27 workout descriptors referencing shared, immutable `{type, seconds}` segment arrays.
Warm-up and cool-down are real segments (index 0 and last) so they're skippable with Next like anything else.

## 6. Cursor rules

- Cursor = index 0–26 of the next-up workout. Persisted in NVS.
- During normal use, only a completion moves the cursor. Confirmed Reset Progress returns it to W1 D1. Starting, paging, pausing, or cancelling never touches it.
- On completion of workout *n*, cursor := *n* + 1 — regardless of where the cursor was before. Repeating W2D1 when the cursor is on W5D3 moves the cursor to W2D2. This is intentional: "wherever I just finished, the next one is next."
- On completion of W9D3, cursor stays at W9D3 (no wrap). Home shows `PROGRAM COMPLETE` at the top.
- Completing an earlier workout again clears that flag and sets the next workout relative to the one just finished. It is not a lifetime achievement flag.
- Partial completions (§7) advance the cursor the same as full ones.

## 7. Logging and persistence

Everything is local to the StopWatch.
The program table is part of the firmware; completed sessions and preferences live separately in nonvolatile flash storage (NVS).
An active workout lives in RAM and is not saved until completion.

### 7.1 Completion log

One entry is appended per completion. Never deduplicated; repeats stack.
Retain the newest 256 entries in a ring; appending entry 257 overwrites the oldest.
This is bounded history, not a permanent archive or lifetime counter.

```cpp
struct __attribute__((packed)) LogEntry {
  uint8_t  workout;      // 0–26
  uint32_t timestamp;    // Unix time at completion, derived from the RTC
  uint16_t total_sec;    // actual active session time, including replayed segments
  uint16_t run_sec;      // actual active time spent in RUN segments, including replays
  uint8_t  flags;        // bit0 = partial (any segment skipped forward)
};
```

- Each packed entry is exactly **10 bytes**, without compiler padding. Millisecond totals are rounded down to whole seconds and saturate at 65,535 seconds; they never wrap.
- Paused time contributes to neither duration. Skipped time is not counted; time spent replaying is counted.
- **Partial** is set if Next was pressed at any point during the session. Track-back (Yellow) does **not** set partial — replaying is not skipping.
- Cancel writes nothing.

### 7.2 Storage contract

Use Arduino `Preferences` backed by ESP32 NVS, with namespace **`c25k`** and key **`state`**.
Store the log, cursor, program-complete flag, and preferences together as one packed **2,578-byte** blob.
The 256 log slots take 2,560 bytes; the header and checksum take 18 bytes.
NVS has its own overhead beyond this payload size.

| Field | Bytes | Meaning |
|---|---:|---|
| `magic` | 4 | Format marker `0x4332354b` |
| `schema` | 1 | Storage schema **1**, independent of this document's version |
| `cursor` | 1 | Next workout, 0–26 |
| `sound_on`, `vib_on` | 2 | One byte each, 0 or 1 |
| `program_complete` | 1 | Whether the most recently completed workout was W9 D3 |
| `reserved` | 1 | Initialized to zero |
| `count` | 2 | Number of retained entries, 0–256 |
| `head` | 2 | Next ring slot to write, 0–255 |
| `entries` | 2,560 | 256 packed `LogEntry` slots |
| `checksum` | 4 | 32-bit FNV-1a over every preceding byte |

The layout is defined in [progress.h](firmware/c25k/progress.h); writes and loading are in [c25k.ino](firmware/c25k/c25k.ino).
The raw packed representation is for this ESP32 target, not a portable export format.
A future format change must define its schema and migration here; a version byte alone is not a migration strategy.

At startup, validate the length, format marker, schema, checksum, and field bounds before using saved data.
If no record exists, start with empty history, W1 D1, and Sound/Vibration on.
If a record is corrupt or uses an unsupported format, retain its bytes, show `STORAGE UNAVAILABLE`, and disable normal writes until an explicit Reset Progress.
The current firmware has no migration from other schemas.

### 7.3 Time

- The RTC clock is separate from the NVS record. Reset Progress does not change it.
- The RTC stores local calendar time. The current build converts it to Unix timestamps using Pacific daylight-saving rules (`C25K_TIMEZONE`, default `PST8PDT,M3.2.0,M11.1.0`). Set and display log dates using the same timezone.
- Settings → Set Time edits hour and minute, preserves a valid existing calendar date, and sets seconds to zero when confirmed.
- If the clock is unset, Set Time uses the firmware build date shown on confirmation. It does not provide a calendar-date or timezone editor. The USB `time UNIX_SECONDS` command can set the full clock while no workout is active; see [BUILD.md](BUILD.md).
- An unset or invalid RTC produces timestamp 0 and `--` in History. Workouts remain available.
- Correct date setup and RTC retention across a real power-off still need the checks listed in §14; a successful software reboot check does not establish either.

### 7.4 Save, reset, and update guarantees

- Save on completion, a Sound/Vibration change, or a confirmed Reset Progress. Do not write every timer tick, page turn, start, pause, or cancellation.
- A completion and its new cursor share one blob save. `Preferences.putBytes()` commits through NVS; read the record back and compare every byte before reporting that save as successful.
- Reset clears all log slots, ring metadata, cursor, and program-complete flag in a candidate record. Preserve Sound and Vibration. Replace the current RAM record only after the candidate save and readback succeed.
- On reset failure, keep the original RAM record available and show the retry screen (§4.6). A write may have succeeded before readback failed; this guarantees retained RAM, not rollback of flash.
- Current limitation: completion and preference changes update RAM before their save is verified. If saving fails, Home reports `STORAGE UNAVAILABLE`, and Complete reports `LOG COULD NOT BE SAVED`. An unsaved change can remain visible until restart or a later successful save. There is no dedicated completion-save retry flow in v1.
- Normal power-off and component firmware updates retain valid saved flash data. Power loss during an active workout loses that unfinished session; reboot loads the last valid persisted record.
- The source-build upload script preserves NVS with the installed partition layout. The full **16 MiB merged image is a fresh-install artifact**; flashing it over the whole device replaces saved flash data. Do not present that download as a history-preserving update.
- Diagnostic workouts must use RAM-only progress or an explicitly isolated test namespace. Production uses `c25k`; the reset persistence check used `c25k_check`. Test setup and cleanup must not reset production history.
- Validate reset persistence by rebooting **immediately after reset, before any other write**. A later settings save must not be able to hide a failed reset.

## 8. Feedback cues

Fire at every segment boundary, whether reached by countdown or by Next/Prev. Both haptic and audio are independently toggleable in Settings.

| Event | Haptic | Audio |
|---|---|---|
| Segment → RUN | 2 buzzes | 2 rising tones |
| Segment → WALK (incl. cool-down) | 1 long buzz | Descending two-note cue |
| 10 s before any boundary | 1 short tick | 1 short tick |
| Pause | 1 short | 1 low tone |
| Resume | 1 short | 1 high tone |
| Complete | 3 short | 3-note ascending |
| Button acknowledged (Next/Prev/Enter) | very short tap | none |

Warm-up start uses the WALK cue. Cues are asynchronous — they never block the timer loop.
The ten-second warning uses the existing Sound and Vibration toggles, with no separate warning toggle.
New segment/state cues replace an unfinished cue; acknowledgement taps do not interrupt a stronger cue.

## 9. Visual style

- True black background, with coral **`#FF765E`** for RUN and aqua **`#52D9CB`** for WALK. Use those colors consistently in summaries, segment labels, rings, and dots; other text is white or grey.
- Center layouts within the actual 468×468 drawing surface and circular aperture. Keep important content at least about 20 px from the circle edge; only progress rings approach the rim. The inscribed square is a guide, not an absolute text boundary.
- Page changes are immediate. Horizontal slide animation is omitted to preserve time for button polling.
- Countdown numerals have a **122 px** glyph height. They must remain legible at arm's length; daylight readability remains an in-hand acceptance check.
- Confirm readable Home previews for a repeated pair, a repeated four-interval sequence, the seven-interval W4 workout, a continuous 30-minute run, and saved completion metadata.

## 10. Timing model

- Timer accounting uses unsigned `millis()` differences and survives its rollover. The RTC supplies completion dates only.
- Update timing before applying a control. Split delayed updates at each segment boundary so each millisecond belongs to the correct activity; ignore time after natural completion.
- Pause excludes time from both the session total and run total. Navigation while paused leaves the timer paused.
- Track-back resets position within a segment and re-arms its warning, while retaining time already spent in actual elapsed/run totals.
- Keep planned sequence position separate from actual active time. The outer arc follows planned position; Complete and History report actual time, including replays.
- `M5.update()` is called every loop iteration; no blocking `delay()` in the main loop so button edges aren't missed.

## 11. Edge cases

- **Power loss mid-workout**: session is lost, nothing logged, cursor unchanged. No resume-in-progress in v1.
- **Battery**: no battery UI during a workout (nothing to act on mid-run). Battery % shown on About.
- **Both buttons pressed but one released early**: apply the exact overlap and release rules in §3; never leak a second single-button click.
- **Next on last segment**: goes straight to Complete, flagged partial.
- **Prev on segment 0**: restart warm-up only; no further back.
- **Enter on Home when a workout page is showing**: always starts that workout, even if it isn't the cursor.

## 12. Build environment

The verified build uses Arduino CLI **1.5.1**, ESP32 core **3.3.10**, M5Unified **0.2.19**, and M5GFX **0.2.26**.
Use target `esp32:esp32:esp32s3` with hardware USB CDC, 16 MiB flash, OPI PSRAM, and `app3M_fat9M_16MB` partitions.
The complete configuration is in [scripts/build.sh](scripts/build.sh); [scripts/flash.sh](scripts/flash.sh) uploads individual components.
The Arduino IDE and PlatformIO alternatives from the original proposal were not used for this verified build.

M5Unified supplies display, buttons, RTC, power, and speaker support, including vibration through `M5.Power.setVibration()`.
No separate M5IOE1 library is required.
Arduino `Preferences` supplies NVS access.

| Source | Responsibility |
|---|---|
| [c25k.ino](firmware/c25k/c25k.ino) | Screen flow, hardware, RTC, verified NVS writes, USB checks |
| [workouts.h](firmware/c25k/workouts.h) | Program descriptors and shared segment arrays (§5) |
| [workout_summary.h](firmware/c25k/workout_summary.h) | Exact activity summaries derived from that table (§4.1) |
| [session.h](firmware/c25k/session.h) | Timing, pause, skip, replay, and cue events (§10) |
| [buttons.h](firmware/c25k/buttons.h) | Four-input grammar (§3) |
| [progress.h](firmware/c25k/progress.h) | Packed data format, history ring, cursor, candidate reset (§7) |
| [feedback.h](firmware/c25k/feedback.h) | Nonblocking audio and vibration cues (§8) |
| [ui.h](firmware/c25k/ui.h) | Circular layouts, summaries, and countdown drawing (§4, §9) |

### 12.1 Acceptance checks

- **Program and summaries:** all 27 durations and run totals match §5; every compressed summary reconstructs the original core exactly.
- **Controls and timing:** chord boundaries, held-gesture exclusivity, pause, replay, skip/partial flags, delayed boundaries, and `millis()` rollover pass the portable checks.
- **Storage:** repeat history, ring overwrite, corrupt-record rejection, cursor rules, and reset failure preserving RAM pass the portable checks.
- **On device:** save a completion and verify it after restart; confirm reset leaves history empty after an immediate restart with no intervening write; retain feedback preferences. Use isolated test data when production history exists.
- **Visuals:** inspect representative round-screen captures, including long interval previews, paused timer, reset confirmation/result, and retained history counts.
- Run [scripts/test.sh](scripts/test.sh) for the portable checks. [BUILD.md](BUILD.md) records device procedures and observed results; §14 keeps the unverified items visible.

## 13. Out of scope for v1

- Touch input
- Screen dimming / sleep during workouts
- Wi-Fi / NTP time sync
- Resume-in-progress after power loss
- Distance, pace, GPS, heart rate
- Custom / editable workouts
- Exporting the log off-device

## 14. Open items

The original implementation choices are resolved: Escape uses 600 ms, and the ten-second warning follows the Sound/Vibration preferences.
Changing either is a product change to this document and its tests.

Remaining acceptance checks:

- Physical button feel and reliability of the 80 ms chord window while moving.
- Perceived audio/vibration cues, including whether the ten-second warning is useful during a run.
- Daylight readability, full-workout battery runtime, and full-session timing drift. The recorded 125-second check is not a substitute for these.
- RTC calendar correctness and retention across a real power-off.

Known limits and possible later work:

- Settings cannot edit the calendar date or timezone. Keep the current build-date fallback and Pacific-time assumption visible until that flow is expanded.
- Completion and preference saves do not have the same retain-before-save behavior as Reset Progress; failures need the handling described in §7.4. A dedicated retry flow remains future work.
- Log export, backup/restore, migration between storage schemas, and resume after power loss remain outside v1. Do not imply a cloud backup exists.

## 15. Revision history

| Version | Date | Change |
|---|---|---|
| [1.0](spec-history/v1.0.md) | 2026-09-11 | Original user-provided design, archived unchanged |
| 1.1 | 2026-09-12 | Make this plan authoritative; add activity previews and explicit reset results; specify the exact storage layout, save/readback behavior, update preservation, clock limits, verified build choices, and remaining acceptance checks |
