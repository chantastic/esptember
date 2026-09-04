# ESPtember — media lesson drafts

Days 03–06 continue from Styled Text: one graphic → a swipeable carousel → a GIF → a movie.
The stopwatch lesson remains a later idea.

These are lesson drafts backed by four implemented, successfully built firmware projects in `days/`.
Hardware verification is in progress.
Verification results belong in `verification.md`; visual checks and SD playback must be distinguished from serial boot checks.
They live outside `days/` so the Astro collection does not publish them.

| Day | Assignment | New idea |
| --- | --- | --- |
| [03: A Static Graphic](day-03-static-graphic/README.md) | Display one image | Convert an asset into firmware data |
| [04: A Carousel](day-04-carousel/README.md) | Swipe between three images | Separate the images from navigation |
| [05: A GIF](day-05-gif/README.md) | Play a short loop | Budget for decoded pixels and a decoder |
| [06: A Movie](day-06-movie/README.md) | Play a clip, then read it from SD | Separate storage, memory, and frame rate |

## Implementation baseline

Use the Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**, the board used by days 01–02.
The M5Stack Stopwatch Dev Kit in the conference CTA is a different product.

The inspected day-02 build uses ESP-IDF v5.5.5, Waveshare BSP 2.0.3, and LVGL 9.5.0.
Pin/reproduce these dependencies for the implementations; the existing component manifest allows updates.
Keep `pmu_init()` and `panel_reset_release()` before `bsp_display_start()`, then the existing brightness call.
All lesson UI setup runs after that bring-up, under the BSP display lock.

Local source checks:

- `days/day-02-styled-text/firmware/managed_components/lvgl__lvgl/lv_version.h`: LVGL 9.5.0.
- `days/day-02-styled-text/firmware/managed_components/lvgl__lvgl/src/widgets/image/lv_image.h`: image API.
- `days/day-02-styled-text/firmware/managed_components/lvgl__lvgl/src/widgets/tileview/lv_tileview.h`: tileview API.
- `days/day-02-styled-text/firmware/managed_components/lvgl__lvgl/src/widgets/gif/lv_gif.c`: GIF defaults, color formats, loading, and loop handling.
- `days/day-02-styled-text/firmware/sdkconfig`: 16 MB flash, RGB565 display, single-app partition table, 64 KB built-in LVGL heap, PSRAM available through malloc.
- `days/day-02-styled-text/firmware/managed_components/waveshare__esp32_s3_touch_amoled_1_8/esp32_s3_touch_amoled_1_8.c`: SDMMC mount in 1-bit mode.
- The BSP header maps SD CMD to GPIO 1, CLK to GPIO 2, and D0 to GPIO 3; its default mount point is `/sdcard`.

## Before turning a draft into a published day

Complete hardware verification for each standalone firmware project and its asset conversion steps.
Build and inspect the app/partition sizes; copy the final excerpt from the actual implementation.
Test on the V2 board, including sustained display operation and each lesson's checks.
Replace proposed behavior with observed behavior where appropriate.
Add a real merged firmware download only after verifying it, then move the README into `days/`.

Day 06 needs separate checks for flash and SD playback.
Its published flash image must include the media if the demo promises playback without a card.
The existing firmware collector copies a merged binary; a new media partition still needs to be added to that merge by the firmware build.
