---
board: waveshare-amoled-18-v2
day: 9
title: Sound Effects Board
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL, esp_codec_dev)
firmware: /firmware/day-09-sound-effects.bin
summary: "Six synthesized sound effects on touch pads, three per page. The board gets a voice."
verification: "Playback pipeline verified over serial; perceived sound pending"
---

## The result

Day 08 made the screen an interface.
Today the board gets a voice: six sound effects on fat orange pads, three per page, played through the ES8311 codec and the onboard speaker.
There are no audio files anywhere in this project — every effect is synthesized from math at press time.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**: ESP32-S3, CO5300 panel, CST816-family touch, ES8311 codec, onboard speaker, 16 MB flash, 8 MB PSRAM.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below. ESP-IDF is not needed.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated. Dependencies are declared in the firmware project.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-09-sound-effects.bin](https://esptember.com/firmware/day-09-sound-effects.bin) and open a terminal in the download directory.
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
  write-flash 0x0 day-09-sound-effects.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## How it works

Sound is just numbers fed to a DAC fast enough.
Each effect is a generator function filling a 16-bit mono buffer at 22050 Hz — a laser is a falling sine sweep, a coin is two square-wave notes, an explosion is lowpassed noise with an exponential decay:

```c
static int gen_boom(int16_t *out)
{
    int n = SAMPLE_RATE * 550 / 1000;
    float low = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        // One-pole lowpass over noise: rumble instead of hiss.
        low += 0.08f * (noise() - low);
        float env = expf(-4.0f * t);
        out[i] = (int16_t)(low * env * 32000);
    }
    return n;
}
```

The codec path is the BSP's three-call story — init I2S, init the speaker codec, write PCM:

```c
    bsp_audio_init(NULL);
    speaker = bsp_audio_codec_speaker_init();
```

One playback task owns the codec.
Pads don't play sounds — they queue a sound id and return immediately, so the UI never blocks on audio.
The queue length is 1 and sends don't wait: mashing pads restarts nothing and stacks nothing; the current effect finishes, the latest request plays next.

Pagination is the day's LVGL rep: two page containers, one visible at a time, a pager row with previous/next and a `1 / 2` indicator.
Hiding a container hides its children — flipping pages is one flag on two objects.

## Check the result

- Three pads on page one: **Laser**, **Coin**, **Boom**. The pager reads `1 / 2`.
- The arrows flip to page two: **Drum**, **Ring**, **Whoosh**.
- Each pad plays its effect through the onboard speaker immediately on tap.

**Recorded evidence · September 21, 2026:** The playback pipeline was verified over the serial harness — pad taps queued the right effect ids, the codec-write path reported active playback, and pagination flipped pages, confirmed by screen capture. Perceived sound quality through the speaker awaits an ear check, noted in NOTES.md.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-09-sound-effects/firmware
idf.py build
idf.py -p PORT flash monitor
```

Exit the monitor with `Ctrl+]`.
To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
The output is `build/merged-binary.bin`.
Keep `pmu_init()` and `panel_reset_release()` before display startup when changing the UI.
New sounds are one generator function and one table entry in `sounds.c` — add a third page by changing `PAGE_COUNT`.
