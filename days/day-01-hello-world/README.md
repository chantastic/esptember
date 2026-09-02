---
day: 1
title: Hello World
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-01-hello-world.bin
---

Put text on the screen.
That's the whole assignment.

Waveshare ships a [BSP](https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_1_8) that powers the panel, starts LVGL, and hands you a canvas.
Hello world is eight calls:

```c
bsp_display_start();
// AMOLEDs have no backlight — this sends the panel its brightness
// command. Required: the one sent during init doesn't stick.
bsp_display_backlight_on();

bsp_display_lock(0);
// Not styling — physics: the dark theme's background is dark grey,
// which keeps every AMOLED pixel lit. True black (#000000) turns
// them off.
lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
lv_obj_t *label = lv_label_create(lv_screen_active());
lv_label_set_text(label, "Hello, ESPtember!");
// Not styling — survival: at the default position (top-left, 0,0)
// the label hides under the panel's rounded corner entirely.
lv_obj_center(label);
bsp_display_unlock();
```

## The board dies in minutes

The code above works.
Then, one to four minutes in, it doesn't.
The screen goes dark — sometimes the whole board with it — and nothing in the log says why.

Two bugs.
Neither is in your eight calls.

### The PMU starves the board

The AXP2101 limits USB input to 500 mA by default.
This board runs an ESP32-S3, lights an AMOLED, and charges a lithium battery on that budget — and when the charger ramps up, the PMU cuts power to everything.

It doesn't look like a power failure.
The screen holds its last frame and the whole thing reads as a software hang.
It isn't.
AMOLED memory keeps the image after death.

Two register writes fix it: raise input to 900 mA, cap charging at 300 mA.
`pmu_init()` in the source.

### The panel reset floats

The wiki says SH8601 panel, FT3168 touch.
Ours boots `co5300` and `CST816S`.
Waveshare revised the hardware and left the docs behind — current units are V2.

On V2, panel reset hides behind a TCA9554 I/O expander the BSP never drives.
A floating reset usually comes up, the demo runs, and everything looks fine.
Then the panel drops dark at random — while every `esp_lcd` call keeps returning `ESP_OK`.

Pulse reset the way the factory firmware does, before display init.
`panel_reset_release()` in the source; [waveshareteam issue #12](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8/issues/12).

With both fixes: a 15-minute soak, no flicker.

## Flash it

Download [day-01-hello-world.bin](https://esptember.com/firmware/day-01-hello-world.bin).
It's the bootloader, partition table, and app merged into one image — one command, no offsets.

Find your port: `ls /dev/cu.usbmodem*` on macOS, `ls /dev/ttyACM*` on Linux.

```sh
uvx esptool --chip esp32s3 --port /dev/cu.usbmodem1101 \
  write-flash 0x0 day-01-hello-world.bin
```

The screen says hello.

Building from source instead?
[ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) v5.5+, then `idf.py -p PORT flash monitor` from `firmware/`.
Dependencies fetch on first build.

## What we learned

`printf` is one line because someone built the plumbing before you arrived.
A screen has no plumbing.
Eight calls is what naive costs on a display.

Defaults are decisions someone else made, for a board they never met.
500 mA starved this one.

Docs describe the board they remember.
The boot log names the board you have.
Trust the log.

Hardware fails politely.
Every call returned `ESP_OK` while the panel sat in reset.
Success codes measure the conversation, not the picture.
