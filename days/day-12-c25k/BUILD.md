# Day 12: Couch to 5K on the M5Stack StopWatch

A standalone, offline interval trainer for the M5Stack StopWatch C152.
The [project plan](SPEC.md) is the source of truth for behavior, all 27 workouts, and the saved-data contract.
This guide covers operating, building, and verifying its implementation.
The firmware is installed on the connected device.

## Use it

Hold the StopWatch with the lanyard loop at the bottom.
The display stays at full brightness, including while paused.

| Gesture | Meaning |
| --- | --- |
| Blue click | Next page or segment |
| Yellow click | Previous page; restart the current segment during a workout |
| Press both, then release | Enter; start a workout, pause/resume, or select |
| Hold both for 600 ms | Escape; Settings from Home, or cancel confirmation during a workout |

Press the two buttons within 80 ms of each other and keep them together for at least 80 ms for Enter.
Release both before the next gesture.
Touch and single-button holds have no actions.

Home opens on the next-up workout, marked by a triangle.
Blue and Yellow browse W1 D1 through W9 D3, followed by History.
Enter starts whichever workout is visible.
On the History landing page, Enter opens the saved sessions, newest first.

Each workout previews its total duration, run/walk times in order, and warm-up and cool-down.
Read the activities left to right, then top to bottom.
Matching sequences show a round count; continuous runs show their full duration.
The preview comes from the same segment table as the timer.

During a workout, Blue skips to the next segment and marks the session partial.
Yellow restarts the current segment; another Yellow within two seconds moves back one segment.
Further Yellow clicks within that interval continue backwards.
Navigation works while paused and leaves the workout paused.

Hold both to show Cancel workout, then release and hold both again to discard it.
The timer keeps running under that overlay unless already paused.
Any click dismisses it; it also disappears after five seconds.
Cancellation saves nothing and leaves the next-up workout unchanged.

Natural completion and skipping past the last segment both save a session.
Partial sessions carry `~`.
Completion advances the next-up workout relative to the workout just completed, so repeating an earlier day can move it backwards.
W9 D3 stays at the end and shows Program complete.

## Settings and time

Hold both on Home to open Settings, then page through Sound, Vibration, Set Time, Reset Progress, and About.
Both selects or toggles; hold both returns home.
Sound and Vibration independently control the boundary cues and the ten-second warning tick.
Reset Progress requires a second Enter and clears only C25K history and its cursor; sound and vibration choices remain.
On the confirmation page, briefly press both buttons and release to confirm; holding both cancels.
Success shows **History cleared**.
Press both again to return Home with W1 D1 next.
If the save cannot be verified, progress remains in memory: press both to retry or hold both to return to Settings.

The RTC was set from this workstation and read back during installation.
This build uses Pacific local time (`America/Los_Angeles`), including daylight-saving rules, to convert the local RTC date/time to Unix log timestamps.
The build-time `C25K_TIMEZONE` definition can be overridden for another region.

Set Time edits hour → minute → confirm while preserving the RTC date.
If the RTC is unset, the confirm screen shows the firmware build date that will be used.
An unset clock does not block a run: its history date is `--`.
For a different calendar date, the USB `time` command below sets the full date/time.
There is no Wi-Fi or NTP connection.

## Build and install

The tested toolchain is Arduino CLI 1.5.1, Espressif ESP32 core 3.3.10, M5Unified 0.2.19, and M5GFX 0.2.26.
M5Unified already includes the StopWatch's M5IOE1 vibration support; a separate M5IOE1 library is unnecessary.

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.10 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install --no-deps "M5Unified@0.2.19" "M5GFX@0.2.26"
cd ~/Developer/esptember
days/day-12-c25k/scripts/build.sh
arduino-cli board list
days/day-12-c25k/scripts/flash.sh /dev/cu.usbmodem1101
```

Replace the port with the connected StopWatch's port if it changes.
The scripts use the verified ESP32-S3 configuration from `~/Developer/m5stack-stopwatch-authkit`: 16 MiB flash, OPI PSRAM, hardware USB CDC, and `app3M_fat9M_16MB` partitions.
The ambiguous USB board name is not the build target.

Use `flash.sh` for component uploads that preserve NVS and filesystem partitions.
The generated 16 MiB merged image is a fresh-install artifact and would overwrite saved data if written across the full flash.
Generated binaries, captures, and reports remain in this day's ignored `.build/` folder.

The isolated source is in `firmware/c25k/`:

| File | Responsibility |
| --- | --- |
| `c25k.ino` | Screen flow, RTC, NVS, hardware setup, main loop, USB checks |
| `workouts.h` | Static program table with real warm-up and cool-down segments |
| `workout_summary.h` | Activity totals and exact repeating sequences derived from the program table |
| `session.h` | Portable timing, pause, skip, track-back, and cue events |
| `buttons.h` | Exclusive Enter/Escape recognition and click swallowing |
| `progress.h` | Packed completion log, ring order, schema, checksum, cursor |
| `feedback.h` | Nonblocking vibration and speaker patterns |
| `ui.h` | Round display layouts and vector countdown numerals |

The additional portable headers keep the timing, button, and storage rules directly testable without an ESP32.
This folder does not depend on the badge application's source or backend.

## Persistence

C25K owns only the `c25k` NVS namespace.
Its 2,578-byte versioned blob holds all settings, the cursor, and a 256-entry circular log.
Each packed log entry is ten bytes.
A completion and its cursor are committed together in one NVS blob.
Every save reads the blob back and compares it with the intended data.
The checksum and bounds checks reject damaged or unsupported data without silently replacing it.
Reset Progress explicitly permits replacing that C25K state.
It adopts the cleared progress in memory only after the save and readback succeed; failure preserves the current in-memory progress for retry.

Log totals count active time spent in each segment, including replays, and exclude paused time.
The outer arc follows the position in the planned workout, so it moves back when a segment is replayed.
The timer uses unsigned `millis()` differences and survives its rollover.
The specified 16-bit log durations saturate at 65,535 seconds for unusually long sessions; ordinary C25K sessions are far shorter.
A power loss discards the in-progress session.

## Verification

Run the portable checks:

```sh
cd ~/Developer/esptember
days/day-12-c25k/scripts/test.sh
```

These compile the production helpers with address and undefined-behavior sanitizers.
They cover all 27 program durations and run totals, boundary cues, pause accounting, replay and skip behavior, button timing thresholds, clock rollover, repeated workouts, cursor movement, ring overwrite, and corrupt storage.
Reset checks verify that a failed save preserves the entire original state and that a successful reset clears progress while retaining feedback preferences.
Summary checks reconstruct all 27 activity sequences from their displayed rounds and durations, verify activity totals, and cover empty, single-segment, and maximum-size inputs.

The installed firmware also exposes a bounded, line-oriented USB protocol at 115200 baud.
`status` reports state; `next`, `prev`, `enter`, and `escape` call the normal screen controls.
`capture` streams a framebuffer snapshot without blocking the timer loop on USB writes.
`time UNIX_SECONDS` updates the complete RTC date/time when no workout is running.
`reboot` checks startup restoration.

`test begin` starts a temporary RAM-only progress store with feedback muted.
`advance MILLISECONDS` advances the real session engine only in this test mode.
`test end` restores the pre-test progress and settings; reboot also returns to saved production data.
These accelerated checks do not measure real-time clock drift or physical button timing.

The repeatable connected-device check is:

```sh
cd ~/Developer/esptember
python3 -m venv days/day-12-c25k/.build/venv
days/day-12-c25k/.build/venv/bin/pip install pyserial pillow
days/day-12-c25k/.build/venv/bin/python days/day-12-c25k/scripts/check-device.py /dev/cu.usbmodem1101
```

It checks every workout through natural completion using accelerated time, captures all screens, tests running/paused navigation and cancel behavior, and verifies the RTC.
When production progress is empty, it also saves one synthetic partial completion and reboots to verify NVS.
It then clears that synthetic history through Reset Progress and reboots immediately, before any later settings write can mask a failed reset.
Finally, it restores the original feedback settings.
Physical interaction invalidates its comparisons; leave the buttons alone while it runs.

Installation checks on September 12, 2026 verified upload hashes, a 468 × 468 framebuffer, 8 MiB PSRAM, speaker initialization, RTC readback, all 27 accelerated completions, and real NVS restoration after reboot.
Captured screens were inspected with a circular display mask.
The Home summary update was checked on all 27 workout pages, with twelve representative captures confirming that the activity sequences fit the round display.
The reset update was checked with two saved test sessions in an isolated `c25k_check` namespace: cancellation kept them, confirmation cleared them, and an immediate reboot kept the history empty and feedback settings intact.
Reinstalling the normal build restored access to the user's two saved sessions, whose History screens matched the pre-test captures.
A separate 125-second real-time check crossed warm-up → RUN → WALK and recorded exactly 60 seconds of running; sampled timer progress tracked host elapsed time within 3 ms.
The largest observed main-loop gap in that timing window was 68.2 ms; framebuffer capture work can make it longer.
This checks a short interval, not full-session clock drift or physical input timing.
Repeat it with `scripts/soak-device.py SERIAL_PORT` using the same Python environment.
Physical button feel, audible/haptic perception, outdoor readability, and battery runtime still need a run with the device in hand.

## Design choices

The active timer uses 122-pixel vector digits, a large segment label, one outer session arc, one inner segment ring, and colored segment dots.
RUN uses coral and WALK uses aqua against true black.
Paused digits remain dimmer while retaining contrast.
The layout follows [Garmin's emphasis on prominent, centered information](https://developer.garmin.com/connect-iq/user-experience-guidelines/incorporating-the-visual-design-and-product-personalities/) and [Wear OS guidance for large countdown numerals](https://developer.android.com/design/ui/wear/guides/styles/typography/apply).

Page changes are immediate; horizontal slide animation is omitted to preserve button polling time.
Long histories show a moving window of dots and an explicit entry number.
The fixed rotation is for handheld stopwatch use, with the loop below the display.

The published [day 12 guide](https://esptember.com/day/day-12-c25k/) and [build story](https://esptember.com/day/day-12-c25k/story/) introduce this firmware.
