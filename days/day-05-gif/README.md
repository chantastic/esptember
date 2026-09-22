---
board: waveshare-amoled-18-v2
day: 5
title: A GIF
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL 9.5)
firmware: /firmware/day-05-gif.bin
summary: "Make Homer disappear into the bushes. A short loop is the whole assignment — and the whole memory lesson."
verification: "Full-screen playback and fills visually confirmed; extended soak pending"
---

## The result

Make Homer Simpson disappear into the bushes.
A short loop is the whole assignment: the carousel moved between separate images, and a GIF brings its images *and their timing* in one file.
The firmware plays a 29-frame full-screen loop forever, edges clean, colors intact.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**: ESP32-S3, CO5300 panel, 16 MB flash, 8 MB PSRAM.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below. ESP-IDF is not needed.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated, plus FFmpeg to convert your own GIF.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-05-gif.bin](https://esptember.com/firmware/day-05-gif.bin) and open a terminal in the download directory.
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
  write-flash 0x0 day-05-gif.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, Homer starts backing up.

## How it works

LVGL 9.5's GIF widget owns the playback — its timer advances the frames, so the application never writes an animation loop:

```c
lv_obj_t *animation = lv_gif_create(lv_screen_active());
// Complete opaque frames need no alpha; use the panel's native format.
lv_gif_set_color_format(animation, LV_COLOR_FORMAT_RGB565);
lv_gif_set_src(animation, &esptember_loop);
```

The color-format line is the day's money line.
At 368 × 448, LVGL's default ARGB8888 decode canvas needs 659,456 bytes; selecting RGB565 — honest for opaque frames on an RGB565 panel — halves it to 329,728.
Either is bigger than day 02's fixed 64 KiB LVGL heap, and the board's 8 MB of PSRAM doesn't enlarge that private pool automatically.
Two lines of sdkconfig route LVGL through the C library allocator, where PSRAM is reachable:

```ini
CONFIG_LV_USE_GIF=y
CONFIG_LV_USE_CLIB_MALLOC=y
```

The asset ships as its original encoded bytes (`EMBED_FILES`), wrapped in a descriptor with `LV_COLOR_FORMAT_RAW` — converting a GIF to a still image would lose the animation, which is the whole point.

`scripts/make-media.py` prepares the loop: scale [the source GIF](https://giphy.com/gifs/the-simpsons-scared-homer-simpson-jUwpNzg9IcyrK) to cover the screen, crop, rebuild the palette with FFmpeg — and, crucially, encode **every frame complete and opaque** (`-gifflags 0`, no transparent palette entry).
`scripts/check-gif-frames.py` rejects any asset that breaks that rule before a build can ship it.

## What went wrong

### Where the fills went

The first frame looked right.
Then the bushes, the wall, and Homer's white shirt turned black while the edges remained.

The encoded GIF marked unchanged pixels as transparent in 28 of its 29 frames — disposal method 1, "keep the previous image underneath" — a standard GIF space optimization.
But the installed LVGL 9.5 drawing code sets those pixels' alpha to zero instead of preserving what was under them, exposing the black screen.
The fix wasn't in the firmware at all: the conversion now flattens every decoded frame to complete, opaque RGB, and the frame validator makes the rule permanent.

## Check the result

- Homer backs into the bushes, full screen, and the loop repeats indefinitely.
- The bushes, wall, and shirt hold their colors through every frame — no black fills.
- The framing sits shifted 26 pixels left, centering Homer.
- Serial logs a `READY` line with the asset's byte counts on boot.

**Recorded evidence · September 4, 2026:** The transparency failure and its fix were observed on hardware; the revised full-screen opaque encoding was visually confirmed (fills correct, playback smooth), with loop callbacks ~3.6 s apart under RGB565 and stable heap/PSRAM across observed heartbeats. An extended soak run remains open, noted in NOTES.md.

## Used resources

- [LVGL GIF widget](https://docs.lvgl.io/9.5/widgets/gif.html) — decoder, timer, and loop events.
- FFmpeg palette rebuild + `-gifflags 0` to disable delta-frame optimizations.
- [Homer backing into the bushes](https://giphy.com/gifs/the-simpsons-scared-homer-simpson-jUwpNzg9IcyrK); source and attribution recorded in the media folder.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-05-gif/firmware
idf.py build
idf.py -p PORT flash monitor
```

To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
For your own GIF: replace `firmware/main/homer.gif`, run the converter and the frame checker, and rebuild — the checker will refuse any file with transparent or partial frames, which is it doing you a favor.
