---
board: m5stack-stopwatch
day: 21
title: Yo
toolchain: Arduino CLI + M5Unified / ESP-IDF v5.5 + Waveshare BSP (one half each)
firmware: /firmware/day-21-yo.bin
summary: "Two boards from different vendors find each other over ESP-NOW and poke. The protocol is the only contract."
verification: "Bidirectional poke exchange machine-verified over both serial consoles"
---

## The result

The single-tap social network, on radios: every board broadcasts a hello, builds a roster of who's nearby, and one press sends a **YO** that buzzes the other device with your name.
It runs across two *different* boards — the M5 StopWatch under Arduino and the Waveshare AMOLED 1.8 under ESP-IDF — because ESP-NOW lives below every framework: no router, no credentials, no TCP, just MAC addresses and small frames.
Nothing above the packet is shared between the two halves, and none of it matters to the radio.
This day has two firmware images, one per board.

## What you need

- **Boards:** any two (or more) of the series' ESP32-S3 kits — the StopWatch half is the featured image; the Waveshare 1.8 half is linked below.
- **Connection:** USB data cables and a computer with access to the serial ports.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** arduino-cli + M5Unified for the StopWatch half; ESP-IDF v5.5 for the Waveshare half.

Flashing replaces the firmware currently on each board.

## Run it

Download both images:
[day-21-yo.bin](https://esptember.com/firmware/day-21-yo.bin) (StopWatch) and
[day-21-yo-waveshare.bin](https://esptember.com/firmware/day-21-yo-waveshare.bin) (AMOLED 1.8).

Find your serial ports:

```sh
# macOS
ls /dev/cu.usbmodem*
# Linux
ls /dev/ttyACM*
```

Flash each board with its image at `0x0`:

```sh
uvx esptool --chip esp32s3 --port STOPWATCH_PORT \
  write-flash 0x0 day-21-yo.bin
uvx esptool --chip esp32s3 --port WAVESHARE_PORT \
  write-flash 0x0 day-21-yo-waveshare.bin
```

Within a few seconds of both booting, each board's roster shows the other.

## How it works

The entire contract between the two codebases is 24 bytes:

```c
typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t type; // 0 = hello, 1 = poke
  char name[12];
} yo_msg_t;
```

Both halves pin the radio to channel 1 in station mode with no connection, broadcast a hello every three seconds, and treat any valid hello as roster material.
Hearing a broadcast requires nothing; *sending back* requires registering the sender as a peer — that asymmetry is ESP-NOW's one etiquette rule, handled on first contact.

A poke is the same struct, `type = 1`, unicast to a chosen MAC.
The send callback reports link-layer delivery — `sent` is not `received` until the radio says acked — and both consoles print the verdict on every frame.

Reception differs by hardware, which is the point: the StopWatch answers a poke with vibration and a tone (day 11's channel), the Waveshare floods its touchscreen orange.
Same packet, native manners.

One cross-framework trap earned its comment: the ESP-NOW receive callback runs on the Wi-Fi task, and M5Unified's vibration is an I²C write — feedback fires from the main loop via a flag, never from the callback.

## Check the result

- Both boards list each other by name within ~6 seconds of boot.
- StopWatch: A selects a peer, B sends YO. Waveshare: tap the peer's row.
- The poked StopWatch buzzes, beeps, and shows `YO! <name>`; the poked Waveshare flashes a full-screen orange `YO! from <name>`.
- Unplug one board: within 10 seconds the other marks it `(gone)`.

**Recorded evidence · September 22, 2026:** The full exchange was machine-verified over both serial consoles simultaneously: mutual roster discovery (`D20_PEER`), StopWatch→Waveshare poke and Waveshare→StopWatch poke each received within the same second (`D20_POKE`/`D20_POKED` pairs), all frames link-acked (`D20_SENT acked`).

## Used resources

- [ESP-NOW](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_now.html) — Espressif's connectionless peer-to-peer protocol, identical under Arduino and ESP-IDF.
- The app **Yo** (2014): the single-tap message as a design ceiling worth respecting.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember/days/day-21-yo
```

StopWatch half:

```sh
./scripts/build-stopwatch.sh
arduino-cli upload --fqbn 'esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi' \
  --port PORT --input-dir .build/firmware firmware/stopwatch/yo_stopwatch
```

Waveshare half (ESP-IDF environment activated):

```sh
cd firmware/waveshare
idf.py build
idf.py -p PORT flash
```

Change `MY_NAME` in each half and flash more boards — the roster holds eight.
The 24-byte struct has room to grow: an emoji field is the obvious v2.
