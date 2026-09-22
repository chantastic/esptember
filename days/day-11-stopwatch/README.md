---
board: m5stack-stopwatch
day: 11
title: Basic-Ass Stopwatch
toolchain: Arduino CLI (esp32 core) + M5Unified
firmware: /firmware/day-11-stopwatch.bin
summary: "The kit is shaped like a stopwatch. Today it behaves like one — real pusher ergonomics included."
verification: "State machine verified over serial; pusher feel pending hands-on"
---

## The result

The kit is shaped like a stopwatch.
Today it behaves like one, with the pusher ergonomics every mechanical stopwatch and Casio digital agrees on: the crown starts and stops — and never resets.
The second pusher laps while running and resets while stopped, and reset requires a hold, because an accidental reset is the one unforgivable stopwatch bug.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** M5Stack Stopwatch Dev Kit (ESP32-S3, C152): two pushers, buzzer, vibration motor, 1.75″ round AMOLED, battery.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [arduino-cli](https://arduino.github.io/arduino-cli/) with the esp32 core and the M5Unified library installed.

The display is 466 × 466 pixels behind a circular aperture.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-11-stopwatch.bin](https://esptember.com/firmware/day-11-stopwatch.bin) and open a terminal in the download directory.
This is a merged image containing the bootloader, partition table, and application.

Find your serial port:

```sh
# macOS
ls /dev/cu.usbmodem*
# Linux
ls /dev/ttyACM*
```

On Windows, use the board's COM port from Device Manager.
Replace `PORT` with your port, then flash the image at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-11-stopwatch.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## How it works

The ergonomics come first, borrowed from a century of mechanical stopwatches and the Casio lap/reset overload:

- **Crown (A): start/stop.** Always. It never resets — a running timer is sacred.
- **Second pusher (B): lap while running, reset while stopped.** Reset is a *hold*, not a tap.

One rule anchors the timekeeping: the display never owns the time.

```c
static uint32_t elapsedMs() {
  return running ? accumulated + (millis() - startedAt) : accumulated;
}
```

Elapsed time is computed from `millis()` deltas against an accumulated base, so rendering rate, button handling, and serial chatter can never skew the clock.
Stopping banks the delta; starting again opens a new one.

Lap freezes a split on screen; the underlying timer never pauses.
The last three laps stack under the main readout, newest highlighted.

Feedback reuses day 10's vocabulary — start is short and high, stop lower, lap a quick chirp, and reset is long and low: the deliberate one.
The screen refreshes at 10 Hz while running and only on events otherwise, straight from day 12's render discipline.

## Check the result

- Crown starts the timer; the readout counts up in `MM:SS.hh`, header goes green **RUNNING**.
- Crown stops it; the time holds. Crown again resumes — no reset.
- **B** while running records a lap without pausing the timer; the last three show on screen.
- **B** tapped while stopped does nothing. **B held** while stopped resets time and laps, with a long low buzz.

**Recorded evidence · September 21, 2026:** Start/stop accumulation, lap capture, and reset were verified over serial (`D11_STATUS` reporting) on the installed firmware. Pusher feel and cue discrimination await a hands-on check, noted in NOTES.md.

## Used resources

- Mechanical stopwatch convention: crown = start/stop, side pusher = lap/reset, reset locked out while running.
- Casio digital watch stopwatch mode: the lap/reset overload on one button, resolved by run state.
- [M5Unified](https://github.com/m5stack/M5Unified) button API: `wasClicked()` / `wasHold()` supply the press vocabulary from day 10.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

Install the toolchain pieces (once):

```sh
arduino-cli core install esp32:esp32
arduino-cli lib install M5Unified
```

Build and flash from the repository root:

```sh
cd days/day-11-stopwatch
./scripts/build.sh
./scripts/flash.sh PORT
```

The merged image lands in `.build/firmware/stopwatch.ino.merged.bin`.
`MAX_LAPS` is 99; the lap list shows three — a scrollable lap history is a natural extension.
