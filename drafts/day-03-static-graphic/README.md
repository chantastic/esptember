---
day: 3
title: A Static Graphic
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL 9.5)
---

> Draft: firmware builds successfully; hardware verification is in progress.
> Build and board results: [verification notes](../verification.md).

Put one graphic on the screen.
Leave it there.

Yesterday we chose the letters and their styles.
Today we choose every pixel.

```c
bsp_display_lock(0);
lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
lv_obj_t *image = lv_image_create(lv_screen_active());
lv_image_set_src(image, &esptember_graphic);
lv_obj_center(image);
bsp_display_unlock();
```

This excerpt is the UI setup from `firmware/main/main.c`.
Board initialization carries over from day 02.

## Make the asset

The included graphic is a 240 × 240 orange ring with a dot in its center.
Its source is `scripts/make-media.py`, which generates the pixels and packs them into little-endian RGB565.
Regenerate the lesson assets from the repository root with `python3 scripts/make-media.py`.
Python makes the still images and raw movie; FFmpeg is also needed for the GIF.

RGB565 stores red, green, and blue in 16 bits per pixel.
It has no alpha channel; our background pixels are black.

The main component embeds the generated binary:

```cmake
idf_component_register(SRCS "main.c" EMBED_FILES "circle.rgb565")
```

ESP-IDF exposes the embedded data as a linker symbol.
The image descriptor tells LVGL what those bytes mean:

```c
extern const uint8_t circle_start[] asm("_binary_circle_rgb565_start");
static const lv_image_dsc_t esptember_graphic = {
    .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                .w = 240, .h = 240, .stride = 480 },
    .data_size = 240 * 240 * 2,
    .data = circle_start,
};
```

For your own PNG, another path is the [LVGL image converter](https://lvgl.io/tools/imageconverter): select LVGL 9, RGB565, no compression, and C array output.
Add the generated C file to `SRCS`, then use its descriptor instead of the one above.
Flatten transparency onto black before converting. [LVGL image documentation](https://docs.lvgl.io/9.4/details/main-modules/image.html)

The artwork is our source.
The pixel data is what we ship.

## Count the pixels

Our 240 × 240 image takes 115,200 bytes of pixel data:

```text
240 × 240 × 2 = 115,200 bytes = 112.5 KiB
```

That excludes the descriptor and any alignment overhead.
It also says nothing about the size of the original PNG.
The compressed file on the computer and the converted pixels on the board have different sizes.

The panel is 368 × 448, so a full-screen RGB565 image takes 329,728 bytes, or 322 KiB, before overhead.
Those dimensions come from the BSP and [Waveshare's board documentation](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.8).

The embedded asset and its constant descriptor live in flash.
The display still needs working buffers in RAM; storing the image in flash doesn't remove those.

## Build and check

The implementation keeps day 02's board bring-up and replaces its text UI with the excerpt above.
Build with `idf.py build` and inspect `idf.py size` before making the merged image.

On hardware, check the image's orientation, the orange against a known color, and the black background.
Let it sit long enough to catch the panel and power problems from day 01.
Record the result before adding the flash download and command here.

## The idea

An image can be part of a program.
Its dimensions become a storage decision.

The screen is small.
The pixels still count.
