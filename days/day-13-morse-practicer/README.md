---
board: m5stack-stopwatch
day: 13
title: Morse Code Practicer
toolchain: Arduino CLI (esp32 core) + M5Unified
firmware: /firmware/day-13-morse-practicer.bin
summary: "A straight key on the crown, ITU timing, live decode. Learn the oldest digital protocol by thumb."
verification: "Boot and decode state machine verified over serial; keying feel pending hands-on"
---

## The result

A straight key on the crown, ITU-R M.1677-1 timing, live decode.
Press A and the sidetone sings; release and the press length decides dit or dah.
Gaps decide letters and words.
The screen shows both the raw symbol stream and the characters it becomes — key `... --- ...` and watch `SOS` assemble itself.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** M5Stack Stopwatch Dev Kit (ESP32-S3, C152): two pushers, buzzer, 1.75″ round AMOLED, battery.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [arduino-cli](https://arduino.github.io/arduino-cli/) with the esp32 core and the M5Unified library installed.

The display is 466 × 466 pixels behind a circular aperture.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-13-morse-practicer.bin](https://esptember.com/firmware/day-13-morse-practicer.bin) and open a terminal in the download directory.
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
  write-flash 0x0 day-13-morse-practicer.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## How it works

Morse is a timing protocol, and the whole standard fits in one comment.
One unit is the dit; dah = 3 units, intra-character gap = 1, letter gap = 3, word gap = 7.
Unit length derives from words-per-minute via the PARIS convention:

```c
static uint32_t unitMs() { return 1200 / wpmSteps[wpmIndex]; }
```

Classification is a midpoint test — a press shorter than 2 units is a dit, longer is a dah, splitting the 1-unit dit from the 3-unit dah with equal tolerance both ways:

```c
static bool isDah(uint32_t pressMs) { return pressMs >= 2 * unitMs(); }
```

The decoder is the same shape as day 11's clock: no counters, just timestamps.
Key releases append symbols; the loop watches the silence after the last release, closing the letter at 3 units and the word at 7.
Nothing blocks — the sidetone starts on press and stops on release, so what you hear is exactly what the decoder measures.

The code table is the 36 ITU letter and digit assignments, looked up when a letter closes.
B taps clear the message; B held cycles keying speed through 5, 10, 15, and 20 WPM — at 5 WPM a dit is a leisurely 240 ms, at 20 it's 60 ms and you'll earn it.

## Check the result

- The screen shows `MORSE 10 WPM`, an empty symbol line, and key hints.
- Pressing A sounds a 600 Hz sidetone for exactly the press duration.
- Short presses append `.`, long presses `-`, drawn large as you key.
- Pause after keying and the symbol collapses into its decoded letter; pause longer and a word space appears.
- `... --- ...` decodes to `SOS`. Unknown patterns decode to `?`.
- B clears; holding B steps the WPM and re-times everything.

**Recorded evidence · September 21, 2026:** Boot and the decode state machine were verified over serial (`D13_STATUS` reporting) on the installed firmware. Keying feel at each WPM step awaits a hands-on check, noted in NOTES.md.

## Used resources

- [ITU-R M.1677-1](https://www.itu.int/rec/R-REC-M.1677-1-200910-I/en), *International Morse code* — the timing ratios (§2) and character assignments.
- The PARIS convention: "PARIS" is 50 units, so unit ms = 1200 / WPM.
- 600 Hz sidetone: the traditional CW listening pitch.

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
cd days/day-13-morse-practicer
./scripts/build.sh
./scripts/flash.sh PORT
```

The merged image lands in `.build/firmware/morse_practicer.ino.merged.bin`.
Natural extensions: a practice mode that shows a target character and scores your timing against the ITU ratios, or iambic keying on both pushers.
