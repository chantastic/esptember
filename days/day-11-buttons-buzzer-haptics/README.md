---
board: m5stack-stopwatch
day: 11
title: Buttons, Buzzer, Haptics
toolchain: Arduino CLI (esp32 core) + M5Unified
firmware: /firmware/day-11-buttons-buzzer-haptics.bin
summary: "No touchscreen. Five button gestures, each with its own tone and vibration — operable from a pocket."
verification: "Boot and serial status verified; button feel pending hands-on"
---

## The result

The StopWatch kit has no touchscreen.
Today teaches the replacement grammar: physical buttons with feedback you can feel and hear without looking.
Five gestures — click A, click B, hold A, hold B, and a two-button chord — each with its own tone and its own vibration, counted live on the round display.
The goal is a device you can operate from a pocket.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** M5Stack Stopwatch Dev Kit (ESP32-S3, C152): two pushers, buzzer, vibration motor, 1.75″ round AMOLED, battery.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [arduino-cli](https://arduino.github.io/arduino-cli/) with the esp32 core and the M5Unified library installed.

The display is 466 × 466 pixels behind a circular aperture.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-11-buttons-buzzer-haptics.bin](https://esptember.com/firmware/day-11-buttons-buzzer-haptics.bin) and open a terminal in the download directory.
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
  write-flash 0x0 day-11-buttons-buzzer-haptics.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## How it works

M5Unified debounces for us and reports clicks on release and holds at a time threshold.
The only decision it can't make alone is the chord: both buttons down together must not also fire two clicks.
The rule comes from day 13's button grammar — a gesture owns its buttons until both release:

```c
    if (locked_) {           // chord fired; swallow everything
      if (!a && !b) locked_ = false;
      return Gesture::None;
    }
    if (a && b) {            // both down = chord, immediately
      locked_ = true;
      return Gesture::Chord;
    }
```

A release after a hold is not a click, either — that's the other leak this grammar plugs.

The feedback map is the actual lesson.
Clicks are short and high, holds are long and low, the chord is the loudest and longest of everything:

```c
//   gesture            tone         vibration
static const uint16_t cueHz[6]    = {0, 880, 660, 440, 330, 1046};
static const uint16_t cueToneMs[6]= {0,  80,  80, 220, 220,  300};
static const uint8_t  cueVibe[6]  = {0, 120, 120, 200, 200,  255};
static const uint16_t cueVibeMs[6]= {0,  60,  60, 220, 220,  320};
```

The point of the mapping is discrimination: with the device unseen, each gesture must feel different from every other.
Tones go through `M5.Speaker.tone()`, vibration through `M5.Power.setVibration()` with a scheduled stop — no blocking delays anywhere in the loop.

## Check the result

- The display shows `DAY 10`, the last gesture in large type, and per-gesture counters.
- Single clicks on either pusher play distinct short high tones with a short buzz.
- Holding either pusher plays a longer, lower tone with a longer vibration.
- Pressing both together fires **CHORD** — highest tone, strongest vibration — and exactly one event, no stray clicks.
- With the screen unseen, the five gestures are distinguishable by feel and sound alone.

**Recorded evidence · September 21, 2026:** Boot, rendering, and the gesture/counter state machine were verified over serial (`D10_STATUS` reporting). Button feel, tone discrimination, and vibration strength await a hands-on check, noted in NOTES.md.

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
cd days/day-11-buttons-buzzer-haptics
./scripts/build.sh
./scripts/flash.sh PORT
```

The merged image lands in `.build/firmware/buttons_buzzer_haptics.ino.merged.bin`.
The cue tables are the obvious place to start changing things — retune the five cues until they're unmistakable in *your* pocket.
