---
board: m5stack-stopwatch
day: 15
title: Music Visualizer
toolchain: Arduino CLI (esp32 core) + M5Unified
firmware: /firmware/day-15-music-visualizer.bin
summary: "A 256-point FFT and 24 radial bars: the round face becomes an equalizer that hears the room."
verification: "Capture + FFT + band dynamics verified over serial; music session pending"
---

## The result

The mic listens, a 256-point FFT splits the room into frequencies, and 24 bars ring the round face like a radial equalizer — bass at twelve o'clock, treble wrapping clockwise.
Bars rise instantly and fall slowly, the decay every hardware visualizer has used since the graphic-EQ era, and their color rides the level from dim orange to white-hot.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** M5Stack Stopwatch Dev Kit (ESP32-S3, C152): onboard microphone (ES8311 codec), 1.75″ round AMOLED, battery.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [arduino-cli](https://arduino.github.io/arduino-cli/) with the esp32 core and the M5Unified library installed.

The display is 466 × 466 pixels behind a circular aperture.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-15-music-visualizer.bin](https://esptember.com/firmware/day-15-music-visualizer.bin) and open a terminal in the download directory.
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
  write-flash 0x0 day-15-music-visualizer.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## How it works

One design constraint shapes the day: M5Unified's speaker and mic share the I2S engine, so today the board only listens.

The FFT is a plain radix-2 Cooley-Tukey, written out instead of imported — it's thirty lines, and seeing it is the lesson.
Two pre-steps matter as much as the transform:

A **Hann window**, because a rectangular window smears every note across the spectrum.
And **DC removal**, because a MEMS mic's bias otherwise leaks through the window into the low bins and pins the bass bars:

```c
  float mean = 0;
  for (int i = 0; i < FFT_N; i++) mean += pcm[i];
  mean /= FFT_N;
  for (int i = 0; i < FFT_N; i++) {
    re[i] = (pcm[i] - mean) * hann[i];
    im[i] = 0;
  }
```

The 24 bars split ~125 Hz to 8 kHz on a **log scale** — equal notes per bar, the spacing ears actually hear.
Each bar takes the peak magnitude in its bin range, converts to dB, and maps ~36 dB of range to bar length.
Bar dynamics are the classic pair: instant rise, slow fall —

```c
    if (level > barLevel[b]) barLevel[b] = level;
    else barLevel[b] = barLevel[b] > 4 ? barLevel[b] - 4 : 0;
```

Rendering reuses day 09's full-screen PSRAM sprite: bars redraw as thick radial strokes every captured frame, glassy-smooth on the round face.

## Check the result

- A quiet room shows a low shimmer of bars breathing with ambient noise.
- Speaking makes the lower-mid bars (upper-left quadrant) jump with your voice.
- A whistle spikes one narrow bar and its neighbors stay down — that's the FFT doing its one job.
- Music lights the whole ring, bass pumping at twelve o'clock, and the slow decay makes beats visible.

**Recorded evidence · September 21, 2026:** Capture, FFT, and band dynamics were verified over serial (`D15_LEVELS` reporting) — ambient levels settled after DC removal and individual bands responded to room sound. A full music session awaits a hands-on check, noted in NOTES.md.

## Used resources

- Cooley-Tukey radix-2 FFT — the 1965 algorithm, thirty lines in this firmware.
- Hann window: the standard anti-leakage taper for audio spectra.
- The graphic-EQ convention: instant attack, slow decay, log-spaced bands.

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
cd days/day-15-music-visualizer
./scripts/build.sh
./scripts/flash.sh PORT
```

The merged image lands in `.build/firmware/music_visualizer.ino.merged.bin`.
Knobs worth turning: `BARS`, the band edges (`fLo`/`fHi`), the dB floor and range in `analyze()`, and the decay rate — every visualizer's personality lives in those numbers.
