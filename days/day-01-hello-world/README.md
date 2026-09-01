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

The display is an SH8601 AMOLED driven over QSPI, powered through an
AXP2101 PMU — none of which you have to touch, because Waveshare ships a
[board support package](https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_1_8)
that brings the panel up and hands you LVGL:

```c
void app_main(void)
{
    bsp_display_start();
    bsp_display_backlight_on();

    bsp_display_lock(0);
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello, ESPtember!");
    lv_obj_center(label);
    bsp_display_unlock();

    // app_main can simply return — the BSP's LVGL task keeps the
    // screen alive without us.
}
```

The whole flashable image is ~570KB — roughly 220KB of bootloader +
hello world, and the rest is the price of the display stack (panel
driver, PMU, LVGL).

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
The BSP dependency is fetched automatically from the component registry
on first build.

```sh
cd firmware
idf.py set-target esp32s3
idf.py -p /dev/cu.usbmodem1101 flash monitor
```

(exit the monitor with `ctrl-]`)

## What's in the image

| offset  | file                | what                                    |
| ------- | ------------------- | --------------------------------------- |
| 0x0     | bootloader.bin      | second-stage bootloader                  |
| 0x8000  | partition-table.bin | where the app lives in flash             |
| 0x10000 | app                 | hello world + display stack (BSP + LVGL) |

The downloadable `.bin` is all three merged into one image flashed at
offset `0x0` — that's why the flash command is a single line.
