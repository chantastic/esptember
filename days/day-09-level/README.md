---
board: m5stack-stopwatch
day: 9
title: Level
toolchain: Arduino CLI (esp32 core) + M5Unified
firmware: /firmware/day-09-level.bin
summary: "The round face becomes a bubble level: tilt readouts, a drifting bubble, and a haptic snap-to-green when it's true."
verification: "IMU tilt tracking verified over serial; flat-surface check pending"
---

## The result

The round face becomes a bubble level, in the spirit of the Apple Watch Ultra's: a bubble that drifts with tilt, live degree readouts for pitch and roll, and a snap-to-green moment — with a haptic tick — when the device lies flat within a degree.
Set it on a shelf and the shelf gets judged.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** M5Stack Stopwatch Dev Kit (ESP32-S3, C152): BMI270 six-axis IMU, vibration motor, buzzer, 1.75″ round AMOLED, battery.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [arduino-cli](https://arduino.github.io/arduino-cli/) with the esp32 core and the M5Unified library installed.

The display is 466 × 466 pixels behind a circular aperture.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-09-level.bin](https://esptember.com/firmware/day-09-level.bin) and open a terminal in the download directory.
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
  write-flash 0x0 day-09-level.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## How it works

The accelerometer's stationary reading is the support force pointing opposite gravity — not gravity itself — and two `atan2`s turn its components into tilt angles:

```c
  const float p = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / (float)M_PI;
  const float r = atan2f(ay, az) * 180.0f / (float)M_PI;
```

Raw accelerometer data trembles, so a low-pass filter steadies the bubble without lag you can feel — one blend line at 15% per sample.

The bubble floats *opposite* the tilt, like the air bubble in a vial: tip the right side down and the bubble escapes left.
Thirty degrees of tilt puts it at the ring; past that it pins to the rim.

The snap-to-level moment uses hysteresis — enter level inside 1.0°, leave outside 1.5° — so the edge never chatters:

```c
  const float worst = fmaxf(fabsf(pitchDeg), fabsf(rollDeg));
  if (!isLevel && worst < LEVEL_TOLERANCE_DEG) {
    isLevel = true;
    M5.Speaker.tone(1046, 60, 0, true);
    M5.Power.setVibration(160); // the tick you feel when it's true
```

Everything renders through a full-screen sprite in PSRAM at ~30 fps: each frame is composed off-screen and pushed whole, so the bubble glides instead of flickering — the first day on this board that animates continuously.

## Check the result

- Face-up on a table: readouts near zero, bubble near center.
- Tilt any direction: the bubble drifts opposite, degree readouts track live, ring stays orange.
- Within a degree of flat: the bubble locks to center, everything snaps green, the buzzer ticks and the motor taps once — **LEVEL**.
- Small wobbles around flat don't flicker the state (hysteresis).

**Recorded evidence · September 21, 2026:** IMU tilt tracking was verified over serial (`D09_STATUS` reporting) — pitch/roll followed physical motion of the board. The snap-to-green threshold against a known-flat reference surface awaits a hands-on check, noted in NOTES.md.

## Used resources

- Apple Watch Ultra's level watch face: the interaction model this borrows — bubble, degrees, snap-to-green.
- [M5Unified](https://github.com/m5stack/M5Unified) IMU API (BMI270) and the support-force convention.
- The [M5StopWatch factory demo](https://github.com/m5stack/M5StopWatch-UserDemo) IMU implementation, for axis orientation reference.

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
cd days/day-09-level
./scripts/build.sh
./scripts/flash.sh PORT
```

The merged image lands in `.build/firmware/level.ino.merged.bin`.
`LEVEL_TOLERANCE_DEG` sets how honest the green is; `RANGE_DEG` sets how twitchy the bubble feels.
An edge-strip inclinometer mode when the device is held vertical is the natural extension.
