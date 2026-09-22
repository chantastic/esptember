---
board: m5stack-stopwatch
day: 19
title: Tempo Checker
toolchain: Arduino CLI (esp32 core) + M5Unified
firmware: /firmware/day-19-tempo-checker.bin
summary: "The metronome's inverse: the mic finds the beat and names the BPM, with a confidence bar that keeps it honest."
verification: "Onset pipeline and estimator verified over serial; music session pending"
---

## The result

The metronome's inverse: the mic listens, onsets are detected as jumps in short-frame energy, the gaps between onsets are folded into musical range, and the median inter-onset interval becomes a BPM — with a confidence bar that keeps the number honest and a dot that nods along with the detected beat.
Point it at day 18's metronome and the pair validates itself: no reference instrument required.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** M5Stack Stopwatch Dev Kit (ESP32-S3, C152): onboard microphone, 1.75″ round AMOLED, battery.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [arduino-cli](https://arduino.github.io/arduino-cli/) with the esp32 core and the M5Unified library installed.

The display is 466 × 466 pixels behind a circular aperture.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-19-tempo-checker.bin](https://esptember.com/firmware/day-19-tempo-checker.bin) and open a terminal in the download directory.
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
  write-flash 0x0 day-19-tempo-checker.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## How it works

A beat is a sudden rise in energy.
Every 16 ms frame gets an RMS energy; an onset fires when energy jumps past 1.8× a slow running average — and at least 220 ms have passed since the last onset, the refractory window that keeps one drum hit from counting twice.
The average adapts at 2% per frame, slow enough that a beat can't raise the bar fast enough to hide its successors.

Tempo comes from the gaps, not the onsets.
Each inter-onset interval is **folded** into the 40–240 BPM window — a missed beat reads as half tempo, so out-of-range gaps double or halve until they land in range:

```c
static uint32_t foldGap(uint32_t ms) {
  while (ms > 1500) ms /= 2;  // < 40 BPM: assume missed beats
  while (ms < 250) ms *= 2;   // > 240 BPM: assume double-counted
  return ms;
}
```

The median folded gap becomes the BPM — day 18's median trick, reused against the world's sloppier taps.
**Confidence** is the fraction of gaps that agree with the median within 12%: steady music scores high, conversation scores near zero, and the display only nods its beat-synced dot above 40%.

Folding is also this lesson's honest limitation: it cannot distinguish 60 BPM from 120 — the octave problem every beat tracker fights.
The confidence bar tells you *a* grid was found; the octave is yours to sanity-check.

## Check the result

- Silence or speech: `--` with a low confidence bar, occasional stray onsets flashing the rim.
- Steady claps: the rim flashes on each clap, the BPM converges to your clapping rate, confidence climbs green.
- Day 18's metronome at 120 BPM playing nearby: the checker reads 120 (or an octave of it), and the nodding dot pulses in step with the clicks.
- B clears the measurement for a fresh song.

**Recorded evidence · September 22, 2026:** The capture, onset detection, folding, and confidence pipeline were verified over serial (`D19_STATUS` reporting) against ambient room sound. The metronome loop-back session awaits a hands-on check, noted in NOTES.md.

## Used resources

- Onset detection by energy flux with a refractory window — the entry-level form of every beat tracker.
- The octave problem: tempo estimators can't distinguish a tempo from its double; folding makes the ambiguity explicit.
- Day 18's metronome as the verification instrument: the pair validates itself.

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
cd days/day-19-tempo-checker
./scripts/build.sh
./scripts/flash.sh PORT
```

The merged image lands in `.build/firmware/tempo_checker.ino.merged.bin`.
Tuning knobs: the 1.8× onset threshold, the 220 ms refractory, the 12% agreement window — every acoustic environment argues for different values, which is the fun.
