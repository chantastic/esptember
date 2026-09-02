---
day: 1
title: Hello World
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-01-hello-world.bin
---

Put text on the screen.

One label.
Default everything.
Big and pretty comes later.

Waveshare ships a [BSP](https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_1_8) that powers the panel and hands you LVGL.
The hello world is six lines:

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

Every line pulls weight.
We deleted three of them.
The screen punished us.
We put them back.

## The board dies in minutes

Flash only the code above and the board runs one to four minutes.
Then it goes dark.

Two bugs.
Both missing from the BSP.

### The PMU starves the board

The AXP2101 limits USB input to 500 mA by default.
ESP32-S3 + AMOLED + battery charging wants more.
When the charger ramps up, the PMU cuts power to everything.

USB disconnects.
The screen freezes on its last frame.
It looks like a software hang.
It isn't — AMOLED memory holds the image after death.

Two register writes fix it: raise input to 900 mA, cap charging at 300 mA.
See `pmu_init()` in the source.

### The panel reset floats

Waveshare revised this board.
Current units ship a CO5300 panel and CST816 touch — not the wiki's SH8601 and FT3168.
Your board is a V2 if the boot log says `co5300` and `CST816S`.

On V2, panel reset hides behind a TCA9554 I/O expander.
The BSP never drives it.
The floating reset usually comes up.
Then it drops dark at random — while every `esp_lcd` call returns `ESP_OK`.

Pulse reset the way the factory firmware does, before display init.
See `panel_reset_release()` in the source, and [waveshareteam issue #12](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8/issues/12).

With both fixes the board ran a 15-minute soak without a flicker.

## What we learned

`printf` is one line because someone built the plumbing before you arrived.
A screen has no plumbing.
Six lines is naive — for a display.

Delete a line and watch what breaks.
The screen teaches faster than the docs.

Hardware lies politely.
Every call returned `ESP_OK` while the panel sat in reset.
A dead AMOLED holds its last frame — a crash that looks like a freeze.

Defaults are decisions someone else made.
500 mA starved this board.

Read your boot log.
Ours said `co5300`, not `SH8601` — a hardware revision the wiki doesn't mention.
Someone already hit your bug.
Search the issues before you burn an afternoon.

## Flash it

You need [esptool](https://docs.espressif.com/projects/esptool/) and the board on USB-C.

Download [day-01-hello-world.bin](https://esptember.com/firmware/day-01-hello-world.bin).

Find your port.
macOS: `ls /dev/cu.usbmodem*`.
Linux: `ls /dev/ttyACM*`.

Flash:

```sh
uvx esptool --chip esp32s3 --port /dev/cu.usbmodem1101 \
  write-flash 0x0 day-01-hello-world.bin
```

The screen says hello.

## Build from source

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) v5.5+.
Dependencies fetch automatically on first build.

```sh
cd firmware
idf.py set-target esp32s3
idf.py -p /dev/cu.usbmodem1101 flash monitor
```

Exit the monitor with `ctrl-]`.

## What's in the image

| offset  | file                | what                          |
| ------- | ------------------- | ----------------------------- |
| 0x0     | bootloader.bin      | second-stage bootloader       |
| 0x8000  | partition-table.bin | where the app lives           |
| 0x10000 | app                 | hello world + display stack   |

The download merges all three into one image flashed at `0x0`.
One command.
No offsets.
