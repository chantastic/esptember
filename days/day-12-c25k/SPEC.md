# C25K for M5Stack StopWatch — Design Spec

Version 1.0 · 2026-09-11

## 1. Purpose

A Couch-to-5K interval trainer that runs natively on the M5Stack StopWatch (C152). Button-only operation, designed to be used mid-run without looking at the screen for anything but a glance. Tracks time running only — no distance, no GPS, no HR.

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

## 3. Input grammar

Four inputs, globally consistent. Every screen uses the same four and only these four.

| Input | Name | Detection |
|---|---|---|
| Blue press | **Next** | BtnB `wasClicked()` with BtnA not held |
| Yellow press | **Prev** | BtnA `wasClicked()` with BtnB not held |
| Both pressed together | **Enter** | Both pressed within ~80 ms of each other, released before hold threshold |
| Both held together | **Escape** | Both held ≥ 600 ms |

Rules:
- Once a "both" gesture is detected, the individual clicks that made it up are swallowed — they never fire as Next/Prev.
- Hold-both and press-both are exclusive: if the hold threshold is crossed, Enter does not fire on release.
- No single-button holds are used anywhere. Keeps the grammar to exactly four.
- Touch input is ignored entirely.

### 3.1 Track-back behavior (in-workout Prev)

Media-player convention:
- Prev press → restart the **current** segment.
- Prev press again within **2 s** of the previous Prev → jump to the **previous** segment.
- Chain continues: each subsequent Prev within 2 s goes back one more segment.
- Cannot go before segment 0 (warm-up); extra presses are no-ops.

### 3.2 Track-forward behavior (in-workout Next)

- Next → advance immediately to the start of the next segment.
- Next on the final segment → ends the workout (goes to Complete screen, flagged partial — see §7).

## 4. Screen architecture

All screens follow the stock-firmware pattern: horizontal paging, one item per page, dot indicators along the bottom arc, current dot filled.

```
                 ┌───────────┐
    Escape ──────┤  SETTINGS ├────── Escape
        │        └───────────┘          │
        ▼                               │
   ┌─────────┐   Enter    ┌──────────┐  │
   │  HOME   ├───────────▶│ WORKOUT  │  │
   │(filmstr)│◀───────────┤ (active) │  │
   └────┬────┘  Complete  └────┬─────┘  │
        │        or            │        │
      Escape   Cancel        Escape     │
        │                      ▼        │
        ▼               ┌────────────┐  │
   ┌─────────┐          │ CANCEL     │  │
   │ HISTORY │          │ CONFIRM    │  │
   └─────────┘          └────────────┘  │
        │                               │
      Escape ───────────────────────────┘
        (back to Home)
```

Top-level toggle: **Escape on Home → Settings. Escape on Settings → Home.**
History is reached by paging past the last workout on Home (see §4.1), and Escape from History returns to Home.

### 4.1 Home

- Horizontal filmstrip of the 27 workouts, W1D1 → W9D3, then one final page: **History**.
- Blue/Yellow page right/left. Wraps at both ends.
- Each workout page shows, inside the inscribed square (~330 px):
  - Large label: `W3 D2`
  - Under it: total session duration (e.g. `28:00`)
  - Under that, small: completion count for this workout and date of last completion (blank if never run). Partial completions shown with a `~` prefix on the count.
- The **cursor** (next-up workout) is marked with a small filled triangle under its page label and its bottom-arc dot is drawn larger/brighter. The cursor is *not* the same as the page you're currently viewing.
- On boot, Home opens on the cursor page.
- **Enter** → start whatever workout is on the current page.
- **Escape** → Settings.

### 4.2 Workout (active)

Layout, top to bottom, all centered:

- Thin **session-progress arc** around the outer edge (full circle = whole session including warm-up/cool-down).
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

Screen: **always on, full brightness** for the duration of a workout. Power-saving deferred to v2.

### 4.3 Cancel Confirm

Overlay on the workout screen (workout keeps timing underneath until confirmed — no auto-pause):

- Text: `Cancel workout?` / `Hold both again to confirm`
- **Escape** → confirm cancel → discard session, nothing logged, return to Home with cursor unchanged.
- **Any other input** (Blue, Yellow, Enter) → dismiss overlay, continue workout.
- Overlay auto-dismisses after 5 s of no input.

### 4.4 Complete

Reached automatically when the final segment's countdown hits zero, or by pressing Next on the final segment.

- Shows `DONE` (or `DONE ~` if partial), workout label, total time, run-time total.
- Log entry is written here (§7).
- Cursor advances here (§6).
- Enter or Escape → Home (opens on the new cursor page).
- Haptic: three short buzzes. Audio: short ascending three-note pattern.

### 4.5 History

- Filmstrip, one log entry per page, newest first.
- Page shows: workout label, date + time (from RTC), total duration, run-time total, `PARTIAL` badge if flagged.
- Blue/Yellow page; wraps.
- Empty state: `No runs yet`.
- **Escape** → Home.
- Enter does nothing (no per-entry actions in v1).

### 4.6 Settings

Filmstrip, one setting per page:

1. **Sound** — On / Off. Enter toggles.
2. **Vibration** — On / Off. Enter toggles.
3. **Set Time** — Enter opens a simple hour → minute → confirm sub-flow using Blue/Yellow to adjust, Enter to advance field, Escape to back out. Needed so log timestamps are right without Wi-Fi.
4. **Reset Progress** — Enter opens a confirm page: `Reset cursor and clear log?` / `Enter to confirm`. Enter → clear; Escape → back. Not needed for repeats (§6) — this is only for starting the whole program over.
5. **About** — firmware version, build date, battery %.

**Escape** on any settings page → Home.

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

Stored in firmware as a static table of `{type, seconds}` arrays, one per workout; warm-up and cool-down are real segments (index 0 and last) so they're skippable with Next like anything else.

## 6. Cursor rules

- Cursor = index 0–26 of the next-up workout. Persisted in NVS.
- **Only a completion moves the cursor.** Starting, paging, pausing, or cancelling never touches it.
- On completion of workout *n*, cursor := *n* + 1 — regardless of where the cursor was before. Repeating W2D1 when the cursor is on W5D3 moves the cursor to W2D2. This is intentional: "wherever I just finished, the next one is next."
- On completion of W9D3, cursor stays at W9D3 (no wrap). Home shows a small `✓ Program complete` line under the label.
- Partial completions (§7) advance the cursor the same as full ones.

## 7. Logging and persistence

### 7.1 Completion log

Append-only. One entry per completion. Never deduplicated; repeats stack.

```
struct LogEntry {
  uint8_t  workout;      // 0–26
  uint32_t timestamp;    // Unix time from RTC
  uint16_t total_sec;    // wall time from start to Complete, excluding paused time
  uint16_t run_sec;      // sum of time spent in RUN segments, excluding paused time
  uint8_t  flags;        // bit0 = partial (any segment skipped forward)
};
```

- **Partial** is set if Next was pressed at any point during the session. Track-back (Yellow) does **not** set partial — replaying is not skipping.
- Cancel writes nothing.
- Storage: NVS blob, ring buffer of 256 entries (~2.3 KB). Oldest overwritten when full.

### 7.2 Other persisted state

- `cursor` (uint8)
- `sound_on`, `vib_on` (bool)
- Schema version byte for forward compatibility.

### 7.3 Time

- RTC keeps time across power-off. Set once via Settings → Set Time.
- If RTC reads as unset (year < 2025), log entries get timestamp 0 and History shows `--` for the date. Nothing else is blocked.

## 8. Feedback cues

Fire at every segment boundary, whether reached by countdown or by Next/Prev. Both haptic and audio are independently toggleable in Settings.

| Event | Haptic | Audio |
|---|---|---|
| Segment → RUN | 2 buzzes | 2 rising tones |
| Segment → WALK (incl. cool-down) | 1 long buzz | 1 falling tone |
| 10 s before any boundary | 1 short tick | 1 short tick |
| Pause | 1 short | 1 low tone |
| Resume | 1 short | 1 high tone |
| Complete | 3 short | 3-note ascending |
| Button acknowledged (Next/Prev/Enter) | very short tap | none |

Warm-up start uses the WALK cue. Cues are asynchronous — they never block the timer loop.

## 9. Visual style

- True black background (AMOLED — off pixels cost nothing).
- Two accent colors: one for RUN, one for WALK. Used for the segment ring, segment label, and the bottom-arc dots. Everything else is white/grey.
- All text inside the inscribed square (466 / √2 ≈ 330 px). Nothing important closer than ~20 px to the circle edge except the rings themselves.
- Filmstrip transitions slide horizontally, ~150 ms, matching stock firmware feel. Can be disabled if it costs frame time.
- Countdown digits should remain legible at arm's length in daylight: target ≥ 110 px tall.

## 10. Timing model

- Timer loop runs off `millis()`, not the RTC. Segment remaining = segment length − (now − segment_start − paused_accum).
- Pause records `pause_start`; resume adds `(now − pause_start)` to `paused_accum`.
- Track-back resets `segment_start = now` and `paused_accum = 0` for the segment.
- `M5.update()` is called every loop iteration; no blocking `delay()` in the main loop so button edges aren't missed.

## 11. Edge cases

- **Power loss mid-workout**: session is lost, nothing logged, cursor unchanged. No resume-in-progress in v1.
- **Battery**: no battery UI during a workout (nothing to act on mid-run). Battery % shown on About.
- **Both buttons pressed but one released early**: if the second button was down < 80 ms, treat as a single click of the first button. Otherwise treat as Enter.
- **Next on last segment**: goes straight to Complete, flagged partial.
- **Prev on segment 0**: restart warm-up only; no further back.
- **Enter on Home when a workout page is showing**: always starts that workout, even if it isn't the cursor.

## 12. Build environment

- Arduino IDE: board option `M5StopWatch`, M5Stack board package ≥ 3.3.7, M5Unified ≥ 0.2.15, M5GFX ≥ 0.2.21.
- Or PlatformIO using the `[env:m5stack-stopwatch]` block from the M5 docs page (espressif32 @ 6.12.0, `esp32s3box` board, `default_16MB.csv` partitions, `qio_opi` memory type).
- Libraries: M5Unified (display, buttons, speaker, power), M5IOE1 (vibration PWM). `Preferences` for NVS.
- Single-sketch layout for v1: `c25k.ino` + `workouts.h` (program table) + `ui.h` (drawing helpers).

## 13. Out of scope for v1

- Touch input
- Screen dimming / sleep during workouts
- Wi-Fi / NTP time sync
- Resume-in-progress after power loss
- Distance, pace, GPS, heart rate
- Custom / editable workouts
- Exporting the log off-device

## 14. Open items

None blocking. Two things to decide during implementation rather than in spec:

1. Exact hold threshold for Escape (600 ms is the starting guess; tune on-device).
2. Whether the 10-second pre-boundary tick is useful or annoying — ship it toggleable under Sound/Vibration or just remove it after a few real runs.
