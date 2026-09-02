---
day: 1
title: Hello World
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-01-hello-world.bin
---

Hello world, on the screen where it belongs: `Hello, ESPtember!` on the
Waveshare ESP32-S3-Touch-AMOLED-1.8's display — the most naive way
possible. One label, default everything, then we're done. (Making it
big and pretty is a later lesson.)

The display is an AMOLED driven over QSPI, powered through an AXP2101
PMU. Waveshare ships a
[board support package](https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_1_8)
that brings up the panel and hands you LVGL — so the hello-world part
is genuinely six lines:

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

Every line is load-bearing — three of them because we deleted them,
watched the screen go wrong, and put them back.

## The part nobody tells you: the board dies in minutes

Flash just the code above and the board runs for one to four minutes,
then goes dark. Two separate bugs, both missing from the BSP:

**1. The PMU starves the board.** The AXP2101 power chip defaults to a
500 mA USB input limit. ESP32-S3 + AMOLED + lithium battery charging
exceeds that once the charger ramps up — and the PMU cuts power to the
*entire system*. USB disconnects; the screen freezes on its last frame
(AMOLED memory holds the image, which makes it look like a software
hang — it isn't). The fix is two register writes: raise the input limit
to 900 mA, cap charging at 300 mA. See `pmu_init()` in the source.

**2. The panel's reset line is floating (V2 boards).** Waveshare
quietly revised this board: current units ship a CO5300 panel driver
and CST816-family touch instead of the V1's SH8601/FT3168. On V2, the
panel reset sits behind a TCA9554 I/O expander that the BSP never
drives. Floating reset means the panel *usually* comes up — then drops
dark at random while every `esp_lcd` call still returns `ESP_OK`. The
fix is the factory firmware's reset pulse, sent before display init.
Known issue:
[waveshareteam/ESP32-S3-Touch-AMOLED-1.8#12](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8/issues/12).
See `panel_reset_release()` in the source.

With both fixes: 15+ minute soak test, rock solid. Your board is a V2
if the boot log says `CST816S` and `co5300`.

## Flash it (prebuilt binary)

You need [esptool](https://docs.espressif.com/projects/esptool/) and the
board connected over USB-C.

1. Download [day-01-hello-world.bin](https://esptember.com/firmware/day-01-hello-world.bin)
2. Find your serial port (macOS: `ls /dev/cu.usbmodem*`, Linux: `ls /dev/ttyACM*`)
3. Flash:

```sh
# with uv (no install)
uvx esptool --chip esp32s3 --port /dev/cu.usbmodem1101 \
  write-flash 0x0 day-01-hello-world.bin

# or with pip-installed esptool
esptool --chip esp32s3 --port /dev/cu.usbmodem1101 \
  write-flash 0x0 day-01-hello-world.bin
```

4. The screen says hello.

## Build from source

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) v5.5+.
Dependencies (the BSP and the TCA9554 I/O expander driver) are fetched
automatically from the component registry on first build.

```sh
cd firmware
idf.py set-target esp32s3
idf.py -p /dev/cu.usbmodem1101 flash monitor
```

(exit the monitor with `ctrl-]`)

## What's in the image

| offset  | file                | what                                     |
| ------- | ------------------- | ---------------------------------------- |
| 0x0     | bootloader.bin      | second-stage bootloader                   |
| 0x8000  | partition-table.bin | where the app lives in flash              |
| 0x10000 | app                 | hello world + display stack (BSP + LVGL)  |

The downloadable `.bin` is all three merged into one image flashed at
offset `0x0` — that's why the flash command is a single line.
