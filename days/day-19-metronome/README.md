---
board: m5stack-stopwatch
day: 19
title: Metronome
toolchain: Arduino CLI (esp32 core) + M5Unified
firmware: /firmware/day-19-metronome.bin
summary: "Drift-free beats from timestamp arithmetic, an accented downbeat you can feel, and tap-tempo on the pusher."
verification: "Beat engine and tap-tempo verified over serial; timing feel pending hands-on"
---

## The result

Day 12 taught that clocks are arithmetic, not counters.
Today that rule keeps time for music: a metronome with an accented downbeat, a haptic click you can feel with the sound off, tap-tempo on the second pusher, and a pendulum dot sweeping the round face — 40 to 240 BPM, 2/4 through 6/8.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** M5Stack Stopwatch Dev Kit (ESP32-S3, C152): two pushers, buzzer, vibration motor, 1.75″ round AMOLED, battery.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [arduino-cli](https://arduino.github.io/arduino-cli/) with the esp32 core and the M5Unified library installed.

The display is 466 × 466 pixels behind a circular aperture.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-19-metronome.bin](https://esptember.com/firmware/day-19-metronome.bin) and open a terminal in the download directory.
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
  write-flash 0x0 day-19-metronome.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## How it works

The naive metronome is `delay(60000 / bpm)` in a loop, and it drifts — every iteration adds rendering time, button handling, whatever the loop cost, and the error compounds forever.
The correct one schedules beats on an absolute timeline:

```c
  if (running && int32_t(now - nextBeatMs) >= 0) {
    click(beatInBar == 0);
    beatInBar = (beatInBar + 1) % beatsPerBar;
    nextBeatMs += beatInterval();
  }
```

The next beat is computed from the last *scheduled* time, never from "now."
If rendering ever ran long, a beat would fire late — but the timeline would not stretch, and the next beat lands back on the grid.
Same principle as day 12's stopwatch, pointed at rhythm.

Tap-tempo takes four taps on the second pusher and uses the **median** gap — the median shrugs off one sloppy tap the way an average can't:

```c
  const uint32_t median = gaps[n / 2];
  int newBpm = (60000 + median / 2) / median;
```

Setting a tempo re-anchors the beat grid to your last tap, so the metronome comes in *on your time*.

The click is layered for no-look use: downbeats are a higher tone and a harder, longer vibration than the other beats — day 11's pitch-duration-intensity code, applied to bars.
The pendulum dot sweeps the arc once per beat, alternating direction like the mechanical original, flashing green on the click.

## Check the result

- A starts the beat immediately; the dot sweeps, pips advance, the downbeat sounds and feels distinctly harder.
- Four taps on B set the tempo to your tapping — the BPM readout updates and the grid re-anchors to your last tap.
- A hold cycles beats per bar (2, 3, 4, 6) and the pip row re-draws to match.
- Sound off (pocket, palm over the speaker): downbeats are still findable by feel alone.
- Against a reference metronome app at 120 BPM, the clicks stay locked — no audible drift over minutes.

**Recorded evidence · September 22, 2026:** The beat engine, tap-tempo math, and transport state were verified over serial (`D18_STATUS` reporting) on the installed firmware. Timing feel and the reference-metronome comparison await a hands-on check, noted in NOTES.md.

## Used resources

- The absolute-schedule pattern (`next += interval`) — the standard fix for accumulating timer drift in game loops and audio schedulers alike.
- Tap-tempo convention: median of recent inter-tap intervals, ~2 s timeout to start fresh.
- Day 11's feedback mapping: pitch encodes identity, duration and intensity encode importance.

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
cd days/day-19-metronome
./scripts/build.sh
./scripts/flash.sh PORT
```

The merged image lands in `.build/firmware/metronome.ino.merged.bin`.
The natural companion is the next lesson: a tempo *checker* that listens back.
