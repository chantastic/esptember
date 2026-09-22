---
board: m5stack-stopwatch
day: 16
title: Tamagotchi
toolchain: Arduino CLI (esp32 core) + M5Unified
firmware: /firmware/day-16-tamagotchi.bin
summary: "A clean-room virtual pet that ages in real time — including while powered off."
verification: "Hatch, meters, and persistence verified over serial; long-term aging pending"
---

## The result

A clean-room virtual pet in the 1996 P1 tradition: hunger and happiness as four hearts each, feeding, snacks, a moody play partner, poop management, and a creature that ages **in real time — including while powered off**.
Power it down for a day and it will have grown, decayed, and possibly pooped in your absence.
The art is original: a 16×16 one-bit creature drawn as fat green pixels in the round face.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** M5Stack Stopwatch Dev Kit (ESP32-S3, C152): two pushers, buzzer, RTC, battery, 1.75″ round AMOLED.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [arduino-cli](https://arduino.github.io/arduino-cli/) with the esp32 core and the M5Unified library installed.

The display is 466 × 466 pixels behind a circular aperture.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-16-tamagotchi.bin](https://esptember.com/firmware/day-16-tamagotchi.bin) and open a terminal in the download directory.
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
  write-flash 0x0 day-16-tamagotchi.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, an egg appears.

## How it works

The rules are hours, compressed from the 1996 pacing: the egg hatches in one hour, childhood arrives at 24, adulthood at 72.
Hunger loses a heart every 4 hours, happiness every 6 — faster if poop is on screen.
A poop appears every 5 waking hours.
Twenty-four consecutive hours with hunger empty and the pet is gone; both pushers held together start a new egg.

The heart of the design is one function:

```c
static void simulate(uint32_t minutes) {
```

The live loop calls it with one minute at a time.
Boot calls it with everything that happened while the power was off:

```c
    const uint32_t away = nowEpoch() - pet.lastSeen;
    if (away > 60) simulate(away / 60);
```

Offline aging is not a special case — it's just a bigger argument.
The RTC supplies real time, NVS keeps the pet through power loss, and every action saves.

Two buttons run the whole toy, one fewer than the original's three: **A cycles** the action menu — FEED, SNACK, PLAY, CLEAN — and **B confirms**.
Feeding fills all hunger hearts.
Snacks buy one happiness heart, the junk-food bargain.
Play is a coin flip against the creature's mood, exactly as capricious as the original's guessing game.
Cleaning removes the poops that were quietly doubling the happiness decay.

The creature itself is five 16-line bitmaps — egg, baby, child, adult, and one we hope you don't meet — hand-authored hex, drawn 14 pixels fat.

## Check the result

- First boot: an egg, `EGG 0h`, four hearts on both meters, and the toast `AN EGG APPEARED`.
- A cycles `< FEED >` through the four actions with a tick; B fires the selected one with its own tone and toast.
- After one hour of real time, the egg hatches — `IT GREW!`.
- Power the board off overnight: on boot, the meters reflect the hours away and the age header has kept counting.
- If the worst happens, both pushers held together lay a new egg.

**Recorded evidence · September 21, 2026:** Egg creation, menu actions, meter changes, and NVS persistence were verified over serial (`D16_STATUS` reporting) on the installed firmware. Multi-day aging, evolution transitions, and death await calendar time, noted in NOTES.md.

## Used resources

- The Tamagotchi P1's documented mechanics — hearts, meal/snack split, guessing-game play, poop cadence, care mistakes — as reimplemented behavior, not copied code or art.
- [TamaLIB](https://github.com/jcrona/tamalib) and MCUGotchi: the emulation route this lesson deliberately didn't take (the real ROM is Bandai's; this pet is ours).
- [M5Unified](https://github.com/m5stack/M5Unified) RTC + Preferences (NVS) for time and persistence.

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
cd days/day-16-tamagotchi
./scripts/build.sh
./scripts/flash.sh PORT
```

The merged image lands in `.build/firmware/tamagotchi.ino.merged.bin`.
The bitmaps are sixteen `uint16_t` rows each — redraw the creature in a hex editor's worth of art.
The pacing constants (`STAGE_AT_HOURS`, the decay minutes) are the difficulty knobs.
