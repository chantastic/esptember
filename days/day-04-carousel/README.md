---
day: 4
title: A Carousel
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL 9.5)
firmware: /firmware/day-04-carousel.bin
---

Put three avatars on the board.
Show one at a time.
Swipe to the next.

Yesterday's image becomes today's slide.
The new work is moving between them.

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

This excerpt uses LVGL 9.5's [Tile View](https://docs.lvgl.io/9.5/widgets/tileview.html).
The tiles occupy columns 0, 1, and 2 in the same row.
Tile View supplies the scrolling and snapping.
The BSP supplies the touch input.

## Reuse the conversion

The included portraits come from my GitHub profile, React Conf speaker page, and React Advanced speaker page.
Same person, three versions of me on the internet.
`scripts/make-media.py` crops each saved photograph to 240 × 240 and converts it to RGB565.
[The media folder](https://github.com/chantastic/esptember/blob/main/assets/media/README.md) records the original files and source links.
The main component embeds `github.rgb565`, `react_conf.rgb565`, and `react_advanced.rgb565`; `main.c` gives each a constant image descriptor, just as in day 03.

Use `CONFIG_LV_USE_TILEVIEW=y`.
It is already enabled in the inspected day-02 configuration; make it explicit in this day's defaults.

Three images cost three images' worth of flash.
At our chosen dimensions, that's 345,600 bytes of pixels, or 337.5 KiB, plus descriptors and the rest of the app.

Full-screen assets would use 966 KiB for the three pixel arrays alone.
Day 02 inherited a 1 MiB app partition, which would leave very little space for the program with those larger assets.
This lesson explicitly reserves a 3 MiB app partition in `partitions.csv`.
The board's 16 MB label doesn't change the partition table.

## Movement can come from code, too

To move to the second slide, call this while holding the display lock:

```c
lv_tileview_set_tile_by_index(carousel, 1, 0, LV_ANIM_ON);
```

That's also the operation an automatic carousel could run from an LVGL timer.
For this day, keep the swipe as the interaction so we can test touch independently of automatic movement.

## Flash it

Download [day-04-carousel.bin](https://esptember.com/firmware/day-04-carousel.bin).
The bootloader, partition table, app, and portraits are merged into one image.

Find your port: `ls /dev/cu.usbmodem*` on macOS, `ls /dev/ttyACM*` on Linux.
Replace the example port below with yours.

```sh
uvx esptool --chip esp32s3 --port /dev/cu.usbmodem1101 \
  write-flash 0x0 day-04-carousel.bin
```

Swipe between the three portraits.
If a swipe does not work, try the programmatic change above to separate tile movement from touch input.

## Build from source

Use [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) v5.5 with the dependencies pinned in this day's firmware project.
From `days/day-04-carousel/firmware/`, run `idf.py -p PORT flash monitor`.
Run `idf.py merge-bin` to produce the combined download image.

## What we learned

The same image descriptor works on the screen or inside a tile.
Navigation belongs to the container, so day 03's portrait becomes a slide without changing its pixels.

Each tile names its available neighbors.
The first and last tiles have one direction each; the middle has two.
That expresses the ends of the gallery in the layout.

Showing one photograph at a time still means storing all three.
Our portraits contribute 345,600 bytes of pixels to the app, and the app has to fit its partition—not just the flash chip.

Movement from code and movement from touch are separate things to verify.
A boot log can tell us the carousel was created; a swipe tells us whether we can use it.
