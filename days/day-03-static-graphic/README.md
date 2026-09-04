---
day: 3
title: A Static Graphic
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL 9.5)
firmware: /firmware/day-03-static-graphic.bin
summary: "Turn a real photograph into pixels the board can display."
verification: "Build verified"
---

## The result

Turn a real photograph into pixels the board can display.
The firmware displays Michael Chan’s saved GitHub portrait at 240 × 240.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**: ESP32-S3, CO5300 panel, CST816-family touch, 16 MB flash, 8 MB PSRAM.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below. ESP-IDF is not needed.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated. Dependencies are declared in the firmware project.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-03-static-graphic.bin](https://esptember.com/firmware/day-03-static-graphic.bin) and open a terminal in the download directory.
This is a merged image containing the bootloader, partition table, and application with its image assets.

Find your serial port:

```sh
# macOS
ls /dev/cu.usbmodem*
# Linux
ls /dev/ttyACM*
```

On Windows, use the board’s COM port from Device Manager.
Replace `PORT` with your port, then flash the image at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-03-static-graphic.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## Check the result

- The grayscale GitHub portrait appears upright, centered, and surrounded by black.
- Compare it with the saved original; check that the crop and tones look correct.
- Leave the image displayed to check for flicker or loss of power.
- In the serial monitor, expect `READY day03` followed by periodic `day03 alive` messages.

Heartbeats confirm that the firmware is running. Inspect the screen to confirm the image.

**Recorded evidence · September 4, 2026:** The portrait firmware builds successfully. Its pixels match the first carousel image; this portrait revision has not been separately flashed.

## How it works

This is the UI excerpt from [the firmware source](https://github.com/chantastic/esptember/tree/main/days/day-03-static-graphic/firmware).
The full program first initializes power, releases the V2 panel reset, starts the display, and sets brightness.

```c
bsp_display_lock(0);
lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
lv_obj_t *image = lv_image_create(lv_screen_active());
lv_image_set_src(image, &esptember_graphic);
lv_obj_center(image);
bsp_display_unlock();
```

The saved portraits and their [source attribution](https://github.com/chantastic/esptember/blob/main/assets/media/README.md) live in the repository.
The converted assets are included; building does not need to fetch a current avatar from the internet.
With Python 3 and FFmpeg installed, regenerate the media lessons from the repository root:

```sh
python3 scripts/make-media.py
```

This regenerates all media assets, including later lessons.
Commit the source image and conversion changes together if you replace a portrait.

The descriptor uses RGB565, width 240, height 240, stride 480, and data size 115,200 bytes.
The component embeds `avatar.rgb565` with `EMBED_FILES`.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-03-static-graphic/firmware
idf.py build
idf.py size
idf.py -p PORT flash monitor
```

Replace `PORT` with the board’s serial port.
Exit the monitor with `Ctrl+]`.
To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
The output is `build/merged-binary.bin`.

Keep `pmu_init()` and `panel_reset_release()` before display startup when changing the UI.

## If it goes wrong

| Symptom | Check / fix |
| --- | --- |
| Image has wrong colors or geometry | Match little-endian RGB565 data to `LV_COLOR_FORMAT_RGB565`, 240 × 240 dimensions, and 480-byte stride. |
| Embedded asset cannot be found | Keep `EMBED_FILES "avatar.rgb565"` in the main component’s CMake configuration. |
| Display goes dark after startup | Preserve the board’s PMU and panel-reset initialization. |

## Take it with you

A JPEG’s file size is not its display budget.
This portrait becomes 115,200 bytes of RGB565 pixels before it reaches the board.

[Read the build story](https://esptember.com/day/day-03-static-graphic/story/) for the investigation and lessons behind this version.
