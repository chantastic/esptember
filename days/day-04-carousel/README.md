---
day: 4
title: A Carousel
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL 9.5)
firmware: /firmware/day-04-carousel.bin
summary: "Swipe through three portraits with LVGL’s Tile View."
verification: "Boot verified"
---

## The result

Swipe through three portraits with LVGL’s Tile View.
The firmware shows Michael Chan’s GitHub, React Conf, and React Advanced portraits, one at a time.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**: ESP32-S3, CO5300 panel, CST816-family touch, 16 MB flash, 8 MB PSRAM.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below. ESP-IDF is not needed.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated. Dependencies are declared in the firmware project.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-04-carousel.bin](https://esptember.com/firmware/day-04-carousel.bin) and open a terminal in the download directory.
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
  write-flash 0x0 day-04-carousel.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## Check the result

- The first slide shows the GitHub portrait. Swipe left to React Conf, then React Advanced.
- Swipe right to return. The first and last slides stop at their outer edges.
- Release halfway through a swipe and check that the view snaps to a slide.
- The serial monitor prints `SWIPE slide=2`, `SWIPE slide=3`, or `SWIPE slide=1` as the selected tile changes.
- Repeat the movement and watch for rendering or touch failures.

`READY day04` confirms creation of the UI. It does not verify physical touch.

**Recorded evidence · September 4, 2026:** The gallery reached READY with unchanged heap readings over 20 seconds. Physical swipes were not captured in that check.

## How it works

This is the UI excerpt from [the firmware source](https://github.com/chantastic/esptember/tree/main/days/day-04-carousel/firmware).
The full program first initializes power, releases the V2 panel reset, starts the display, and sets brightness.

```c
const lv_image_dsc_t *slides[] = { &github, &react_conf, &react_advanced };
bsp_display_lock(0);
lv_obj_t *carousel = lv_tileview_create(lv_screen_active());
lv_obj_set_size(carousel, lv_pct(100), lv_pct(100));
lv_obj_set_style_pad_all(carousel, 0, 0);
lv_obj_set_style_border_width(carousel, 0, 0);
lv_obj_set_style_bg_color(carousel, lv_color_black(), 0);
lv_obj_set_scrollbar_mode(carousel, LV_SCROLLBAR_MODE_OFF);
for (uint8_t i = 0; i < 3; i++) {
    lv_dir_t directions = LV_DIR_NONE;
    if (i > 0) directions |= LV_DIR_LEFT;
    if (i < 2) directions |= LV_DIR_RIGHT;
    lv_obj_t *tile = lv_tileview_add_tile(carousel, i, 0, directions);
    lv_obj_set_style_pad_all(tile, 0, 0);
    lv_obj_t *image = lv_image_create(tile);
    lv_image_set_src(image, slides[i]);
    lv_obj_center(image);
}
lv_obj_add_event_cb(carousel, slide_changed, LV_EVENT_VALUE_CHANGED, NULL);
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

Three 240 × 240 RGB565 portraits consume 345,600 bytes before descriptors and program code.
`CONFIG_LV_USE_TILEVIEW=y` enables the widget.
Each tile contains one image and names its available neighbors.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-04-carousel/firmware
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
| Swipe does nothing | While holding the display lock, try `lv_tileview_set_tile_by_index(carousel, 1, 0, LV_ANIM_ON)`. If it moves, investigate touch input. |
| Gallery moves beyond an end | Check each tile’s allowed directions: right on the first, both on the middle, left on the last. |
| App no longer fits after adding images | Inspect `idf.py size` and `partitions.csv`. This project reserves 3 MiB for the app; total flash capacity is a different budget. |

## Take it with you

Showing one image still requires storing all three.
The container handles navigation; the descriptors and pixel data remain the same as a static image.

[Read the build story](https://esptember.com/day/day-04-carousel/story/) for the investigation and lessons behind this version.
