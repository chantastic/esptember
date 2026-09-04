---
day: 2
title: Styled Text
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-02-styled-text.bin
summary: "Make the greeting yours with color, size, and a custom font."
verification: "Rendering observed"
---

## The result

Make the greeting yours with color, size, and a custom font.
The firmware displays an orange, bold italic greeting on black.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**: ESP32-S3, CO5300 panel, CST816-family touch, 16 MB flash, 8 MB PSRAM.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below. ESP-IDF is not needed.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated. Dependencies are declared in the firmware project.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-02-styled-text.bin](https://esptember.com/firmware/day-02-styled-text.bin) and open a terminal in the download directory.
This is a merged image containing the bootloader, partition table, and application.

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
  write-flash 0x0 day-02-styled-text.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## Check the result

- The greeting appears in orange (`#ff5b04`) on black.
- The type is 48px Montserrat bold italic, wrapped and centered inside 90% of the screen width.
- Every character is visible; no blank label or clipped line appears.

**Recorded evidence · September 4, 2026:** The custom-font rendering failure and its fix were observed during development. No separate timed soak is recorded for this lesson.

## How it works

This is the UI excerpt from [the firmware source](https://github.com/chantastic/esptember/tree/main/days/day-02-styled-text/firmware).
The full program first initializes power, releases the V2 panel reset, starts the display, and sets brightness.

```c
LV_FONT_DECLARE(montserrat_bi_48);

bsp_display_lock(0);
// True black — on an AMOLED the background is off, not painted.
lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);

lv_obj_t *label = lv_label_create(lv_screen_active());
lv_label_set_text(label, "Hello, ESPtember!");
// Color: one line.
lv_obj_set_style_text_color(label, lv_color_hex(0xff5b04), 0);
// Typeface: the converted font above. Built-ins stop at regular.
lv_obj_set_style_text_font(label, &montserrat_bi_48, 0);
// 48px type doesn't fit a 368px panel on one line — typography
// is layout. Wrap at 90% width, center the lines.
lv_obj_set_width(label, lv_pct(90));
lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
lv_obj_center(label);
bsp_display_unlock();
```

The converted font is already included in `firmware/main/montserrat_bi_48.c`.
To regenerate it, provide `Montserrat-BoldItalic.ttf` in that directory and run:

```sh
npx lv_font_conv --font Montserrat-BoldItalic.ttf \
  --size 48 --bpp 4 --range 0x20-0x7E \
  --format lvgl --lv-include lvgl.h --no-compress \
  -o montserrat_bi_48.c
```

The ASCII range supplies the characters used here.
The `--no-compress` flag matches this firmware’s decoder configuration.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-02-styled-text/firmware
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
| Custom font compiles but the label is blank | Convert with `--no-compress`, as this project does. The current LVGL configuration has font decompression disabled. |
| Text clips at the screen edge | Set the label width to 90% and center its text before centering the object. |
| Display goes dark or board loses power | Preserve the PMU and panel-reset setup from day 01. |

## Take it with you

Fonts become firmware data.
Choose the glyphs, size, and compression your build supports, then give the text enough room to render.

[Read the build story](https://esptember.com/day/day-02-styled-text/story/) for the investigation and lessons behind this version.
