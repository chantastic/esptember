---
day: 5
title: A GIF
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL 9.5)
---

> Draft: firmware builds successfully; hardware verification is in progress.
> Build and board results: [verification notes](../verification.md).

Make a graphic move without touching the screen.
A short loop is the whole assignment.

The carousel moved between separate images.
A GIF brings its images and their timing in one file.

```c
bsp_display_lock(0);
lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
lv_obj_t *animation = lv_gif_create(lv_screen_active());
lv_gif_set_src(animation, &esptember_loop);
if (lv_gif_is_loaded(animation)) {
    lv_gif_set_loop_count(animation, 0);
    lv_obj_center(animation);
    lv_obj_add_event_cb(animation, loop_finished, LV_EVENT_READY, NULL);
    ESP_LOGI("media", "READY day05: GIF loaded; bytes=%lu; ARGB8888 canvas=102400 bytes",
             (unsigned long)esptember_loop.data_size);
} else {
    ESP_LOGE("media", "GIF load failed");
    lv_obj_t *error = lv_label_create(lv_screen_active());
    lv_label_set_text(error, "GIF failed to load");
    lv_obj_center(error);
}
bsp_display_unlock();
```

The excerpt uses LVGL 9.5's GIF widget.
Its timer advances the frames; the application doesn't need its own animation loop. [LVGL GIF documentation](https://docs.lvgl.io/9.5/widgets/gif.html)

## Prepare the loop

The included GIF shows an orange dot orbiting a gray ring on black.
`scripts/make-media.py` generates twenty 160 × 160 frames and asks FFmpeg to assemble them at ten frames per second.
It writes `firmware/main/orbit.gif`.

The component embeds the GIF with `EMBED_FILES "orbit.gif"`.
The application wraps the original bytes in an `lv_image_dsc_t` with `LV_COLOR_FORMAT_RAW`, then passes that descriptor to the widget.

For your own GIF, replace `orbit.gif` and rebuild, updating the descriptor dimensions if needed.
The [LVGL image converter](https://lvgl.io/tools/imageconverter) also supports a Raw C array workflow; Raw preserves the encoded GIF for its decoder.
Converting it to a still RGB565 image would lose the animation.

Enable the widget and select the C library allocator in `sdkconfig.defaults`:

```ini
CONFIG_LV_USE_GIF=y
# CONFIG_LV_USE_BUILTIN_MALLOC is not set
CONFIG_LV_USE_CLIB_MALLOC=y
```

Keep the existing PSRAM configuration, including `CONFIG_SPIRAM_USE_MALLOC=y` in the effective configuration.
If reusing a build directory, change the allocator choice in `idf.py menuconfig` too; an existing `sdkconfig` takes precedence over defaults.

## A small file can need a bigger canvas

The inspected LVGL 9.5 implementation defaults to ARGB8888 for its GIF canvas.
At 160 × 160, that canvas alone needs 102,400 bytes before alignment and decoder overhead.

Day 02's LVGL configuration uses a fixed 64 KiB heap.
The GIF canvas is already larger than that pool.
The board's 8 MB of PSRAM doesn't enlarge LVGL's private allocator automatically.

Switching LVGL to the C library allocator lets it use the ESP-IDF heap configuration.
Measure available memory and check allocation success on the board before claiming the GIF fits.

For an opaque GIF, LVGL 9.5 also supports setting the canvas to RGB565 **before** loading the source:

```c
lv_gif_set_color_format(animation, LV_COLOR_FORMAT_RGB565);
```

That makes a 160 × 160 canvas 51,200 bytes before overhead.
It changes the decoded canvas, not the size of the GIF stored in flash.
Keep ARGB8888 for the first implementation, then compare the opaque asset in RGB565 as a separate experiment.

These allocator names and canvas behavior were checked against the installed LVGL 9.5 source.
Older GIF examples have different memory formulas; use the version you're building.

## Build and check

Build the project and confirm `lv_gif_is_loaded()` succeeds on hardware.
Have the implementation log an error and show a message if loading fails.
Check that the animation repeats beyond its original loop count, that the edges stay clean, and that memory remains stable over repeated loops.

If this asset uses transparency or partial-frame disposal later, test those explicitly.
The installed decoder's handling of disposal method 3 is marked unsupported in source.

Add measured memory use, the verified firmware download, and flash instructions after the test.

## The idea

The file is what we store.
The canvas is what we draw.

Compression can shrink the first without shrinking the second.
