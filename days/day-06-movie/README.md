---
board: waveshare-amoled-18-v2
day: 6
title: A Movie
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL 9.5)
firmware: /firmware/day-06-movie.bin
summary: "Play a movie from raw frames — then make it longer than the board can hold, with an SD card."
verification: "10 fps flash playback observed over serial; SD path unverified"
---

## The result

Play a movie on the board.
Then make it longer than the board can hold.

The player draws raw RGB565 frames — 368 × 224, ten per second, no audio — read one at a time from a dedicated flash partition, or from a microSD card when one is present.
The computer does the conversion; the board only reads and draws.
The included clip is three seconds of *Big Buck Bunny*, looping.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**: ESP32-S3, 16 MB flash, 8 MB PSRAM, onboard microSD slot.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below. ESP-IDF is not needed.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated, plus [FFmpeg](https://ffmpeg.org/ffmpeg.html) to convert movies.
- **Optionally:** a FAT32 microSD card for clips longer than flash allows.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-06-movie.bin](https://esptember.com/firmware/day-06-movie.bin) and open a terminal in the download directory.
This is a merged image containing the bootloader, partition table, application, and the media partition.

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
  write-flash 0x0 day-06-movie.bin
```

When flashing completes, the bunny plays.

## How it works

Two frame buffers in PSRAM split the work: one receives the next frame from storage, the other holds the pixels LVGL is allowed to draw.
The read happens outside the display lock so the UI keeps drawing the previous frame; the swap happens inside it:

```c
// LVGL must never draw a frame while we are overwriting its pixels.
bsp_display_lock(0);
lv_image_cache_drop(&frame);
memcpy(visible, staging, FRAME_BYTES);
lv_obj_invalidate(image);
bsp_display_unlock();
```

The raw format carries no metadata — width, height, and rate are a contract between the FFmpeg command and the firmware constants:

```sh
ffmpeg -i input.mp4 -t 3 -an \
  -vf "fps=10,scale=368:224:force_original_aspect_ratio=decrease,pad=368:224:(ow-iw)/2:(oh-ih)/2:black,setsar=1" \
  -c:v rawvideo -pix_fmt rgb565le -f rawvideo movie.rgb
```

The budgets are the lesson.
One frame is 164,864 bytes; a second is 1.6 MB; the three-second clip is 4,945,920 bytes — far past the default 1 MiB app partition, so this day introduces a **custom partition table**: 3 MiB for the app and a 12 MiB raw `media` partition, no filesystem overhead.
Twelve MiB holds 7.6 seconds at this format's rate — which is exactly why the player also mounts the **onboard microSD slot** (`bsp_sdcard_mount()`, SDMMC 1-bit): convert without `-t 3`, drop `movie.rgb` on a FAT32 card, and the storage ceiling moves from megabytes to gigabytes.
One minute of raw frames is ~94 MiB; the two RAM buffers never grow.
If the card or file is missing, the firmware falls back to the flash clip — watch serial for `source=SD` or `source=flash`, because a successful fallback looks exactly like a successful SD test.

## What went wrong

### Resize before playback

The first attempt stored 184 × 224 frames and asked LVGL to scale them 2× on the board.
Submissions fell to about 3.7 fps with repeated missed deadlines — the scaler ate the frame budget.
Converting to full 368-pixel width on the computer doubled the file and *removed* the on-board work: the native-size build reported 10.01 fps with one initial late deadline, then 10.00 fps with zero late frames in the following windows.
Resize where the CPU is cheap.

## Check the result

- The bunny clip plays at ten frames per second and loops at the boundary.
- Serial reports the submission rate, late-frame count, and `source=flash` (or `source=SD` with a card).
- With a longer `movie.rgb` on a FAT32 card: the card's clip plays instead.
- A missing card or file falls back to the flash clip and says so.

**Recorded evidence · September 4, 2026:** Native-size flash playback was observed over serial at 10.0 fps with stable heap and PSRAM across the reporting windows (submission rate, not certified panel frames). The SD path is **unverified** — the one mount attempt returned a timeout and the flash fallback played; card compatibility and sustained reads remain open, noted in the verification notes.

## Used resources

- [FFmpeg rawvideo](https://ffmpeg.org/ffmpeg-formats.html#rawvideo) — the no-metadata contract.
- [ESP-IDF partition tables](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-guides/partition-tables.html) — the 12 MiB raw media partition.
- *Big Buck Bunny* © 2008 Blender Foundation / [bigbuckbunny.org](https://www.bigbuckbunny.org/), [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) — trimmed, silenced, resized; details with the saved source.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-06-movie/firmware
idf.py build
idf.py -p PORT flash monitor
```

`idf.py merge-bin` produces the single image including the media partition — check that the media bytes land at `0x310000`.
To change the clip, replace `firmware/main/movie.rgb` and rebuild; the build checks the partition's capacity and the app stops at the clip's true length.
The next step this player wants is MJPEG — compressed frames, a JPEG decoder, and roughly ten times the runtime per megabyte; the README's conversion command for that future is in the source tree.
