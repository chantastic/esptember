---
board: waveshare-amoled-18-v2
day: 5
title: A GIF
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL 9.5)
---

> Draft: firmware builds successfully; hardware verification is in progress.
> Build and board results: [verification notes](../verification.md).

Make Homer Simpson disappear into the bushes.
A short loop is the whole assignment.

The carousel moved between separate images.
A GIF brings its images and their timing in one file.

```c
bsp_display_lock(0);
lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
lv_obj_t *animation = lv_gif_create(lv_screen_active());
// Complete opaque frames need no alpha; use the panel's native format.
lv_gif_set_color_format(animation, LV_COLOR_FORMAT_RGB565);
lv_gif_set_src(animation, &esptember_loop);
if (lv_gif_is_loaded(animation)) {
    lv_gif_set_loop_count(animation, 0);
    lv_obj_center(animation);
    lv_obj_add_event_cb(animation, loop_finished, LV_EVENT_READY, NULL);
    ESP_LOGI("media", "READY day05: Homer GIF loaded; bytes=%lu; RGB565 canvas=329728 bytes",
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

The included GIF is [Homer backing into the bushes](https://giphy.com/gifs/the-simpsons-scared-homer-simpson-jUwpNzg9IcyrK).
`scripts/make-media.py` takes the saved 500 × 375 GIF, scales it to cover the 368 × 448 screen, crops the sides with the picture shifted 26 pixels left (7% of the screen width), and rebuilds its palette with FFmpeg.
Every encoded frame is complete and opaque so unchanged colors survive playback.
It writes `firmware/main/homer.gif`: 29 frames with 100 ms delays, 2.9 seconds of encoded animation.
[The media folder](../../assets/media/README.md) records the source and attribution.

The component embeds the GIF with `EMBED_FILES "homer.gif"`.
The application wraps the original bytes in an `lv_image_dsc_t` with `LV_COLOR_FORMAT_RAW`, then passes that descriptor to the widget.

For your own GIF, replace `homer.gif` and rebuild, updating the descriptor dimensions if needed.
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

## Budget for the file and the canvas

We select RGB565 before loading the GIF because these frames are opaque and the panel uses RGB565.
At 368 × 448, that canvas needs 329,728 bytes before alignment and decoder overhead.
LVGL 9.5's default ARGB8888 canvas would need 659,456 bytes.
The first full-screen ARGB8888 test took about 4.9 seconds per loop, compared with 2.9 seconds encoded in the GIF.

Day 02's LVGL configuration uses a fixed 64 KiB heap.
Either canvas is larger than that pool.
The board's 8 MB of PSRAM doesn't enlarge LVGL's private allocator automatically.

Switching LVGL to the C library allocator lets it use the ESP-IDF heap configuration.
Changing the canvas format reduces the decoded buffer and drawing work; it does not change the encoded GIF's size in flash.

These allocator names and canvas behavior were checked against the installed LVGL 9.5 source.
Older GIF examples have different memory formulas; use the version you're building.

## Where the fills went

The first frame looked right.
Then the bushes, wall, and Homer's white shirt turned black while edges remained.

The encoded GIF marked unchanged pixels as transparent in 28 of its 29 frames.
Those frames used disposal method 1: keep the previous image underneath.
But the installed LVGL 9.5 ARGB8888 drawing code sets those pixels' alpha to zero, exposing the black screen instead of keeping their previous colors.

The conversion now flattens the decoded frames to RGB, reserves no transparent palette entry, and disables FFmpeg's GIF optimizations with `-gifflags 0`.
Every frame supplies all its own colors.
`scripts/check-gif-frames.py` checks the resulting file and rejects transparent or partial frames before a build.

## Build and check

Build the project and confirm `lv_gif_is_loaded()` succeeds on hardware.
Have the implementation log an error and show a message if loading fails.
Check that the animation repeats beyond its original loop count, that the edges stay clean, and that memory remains stable over repeated loops.

If this asset uses transparency or partial-frame disposal later, test those explicitly.
The installed decoder's handling of disposal method 3 is marked unsupported in source.

The full-screen RGB565 revision produced loop callbacks about 3.6 seconds apart in the short serial check.
The full-screen opaque version was visually confirmed on the board: the bushes, wall, and shirt retain their colors.
The framing was then shifted 26 pixels left at the user’s request.
An extended run still comes before publication.

## What we learned

A GIF can use transparency to mean “keep the previous pixel.”
Our first conversion did that in 28 of 29 frames, but this LVGL version cleared those pixels instead.
Complete opaque frames preserved the bushes, wall, and shirt; the conversion now checks that every frame follows that rule.

The encoded file and decoded canvas need separate budgets.
Using RGB565 for our opaque full-screen animation halved the canvas from 659,456 to 329,728 bytes and shortened observed loop callbacks from about 4.9 seconds to 3.6 seconds.
Both canvases exceed the old 64 KiB LVGL heap, even though the board has 8 MB of PSRAM.

Filling the screen is a framing decision.
We preserved the aspect ratio, cropped the sides, and shifted the picture 26 pixels left to center Homer more closely.

The logs kept counting loops while the colors were wrong.
A running animation still needs someone to look at it.
