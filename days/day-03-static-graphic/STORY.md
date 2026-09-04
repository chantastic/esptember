---
title: A Static Graphic
---


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

The included graphic is my GitHub avatar, cropped to 240 × 240.
`scripts/make-media.py` converts the saved JPEG into little-endian RGB565 with FFmpeg.
Regenerate the lesson assets from the repository root with `python3 scripts/make-media.py`.
The original JPEG and its source link live in [the media folder](https://github.com/chantastic/esptember/blob/main/assets/media/README.md).

RGB565 stores red, green, and blue in 16 bits per pixel.
It has no alpha channel; the photograph supplies every pixel.

The main component embeds the generated binary:

```cmake
idf_component_register(SRCS "main.c" EMBED_FILES "avatar.rgb565")
```

ESP-IDF exposes the embedded data as a linker symbol.
The image descriptor tells LVGL what those bytes mean:

```c
extern const uint8_t avatar_start[] asm("_binary_avatar_rgb565_start");
static const lv_image_dsc_t esptember_graphic = {
    .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                .w = 240, .h = 240, .stride = 480 },
    .data_size = 240 * 240 * 2,
    .data = avatar_start,
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
It also says nothing about the size of the original JPEG.
The compressed file on the computer and the converted pixels on the board have different sizes.

The panel is 368 × 448, so a full-screen RGB565 image takes 329,728 bytes, or 322 KiB, before overhead.
Those dimensions come from the BSP and [Waveshare's board documentation](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.8).

The embedded asset and its constant descriptor live in flash.
The display still needs working buffers in RAM; storing the image in flash doesn't remove those.

## What we learned

The JPEG becomes pixels before the board sees it.
Converting on the computer lets the firmware display a photograph without adding a JPEG decoder.

Dimensions give us an exact pixel budget.
Our 240 × 240 RGB565 portrait uses 115,200 bytes, regardless of how small the source JPEG was.

The descriptor is part of the image: format, dimensions, stride, and data must agree.
Saving the original portrait alongside the conversion script also lets us rebuild this version after the online avatar changes.

The screen is small.
The pixels still count.
