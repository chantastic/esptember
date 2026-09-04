---
board: waveshare-amoled-18-v2
day: 1
title: Hello World
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-01-hello-world.bin
summary: "Put your first words on an AMOLED. Keep them there."
verification: "Visual soak recorded"
---

## The result

Put your first words on an AMOLED. Keep them there.
The firmware displays “Hello, ESPtember!” in the center of a black screen.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**: ESP32-S3, CO5300 panel, CST816-family touch, 16 MB flash, 8 MB PSRAM.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below. ESP-IDF is not needed.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated. Dependencies are declared in the firmware project.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-01-hello-world.bin](https://esptember.com/firmware/day-01-hello-world.bin) and open a terminal in the download directory.
This is a merged image containing the bootloader, partition table, and application.

Find your serial port:

```sh
# macOS
ls /dev/cu.usbmodem*
# Linux
ls /dev/ttyACM*
```

On Windows, use the board’s COM port from Device Manager.
Replace `PORT` with your port, then flash the image at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-01-hello-world.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## Check the result

- “Hello, ESPtember!” appears in the center on a black background.
- The label remains visible and USB stays connected during a 15-minute run.
- No flicker, unexpected freeze, or power loss occurs.

A successful flash or `ESP_OK` response does not establish that the panel is drawing.

**Recorded evidence · September 4, 2026:** The original lesson records a 15-minute hardware soak with both board fixes applied and no flicker.

## How it works

This is the UI and startup excerpt from [the firmware source](https://github.com/chantastic/esptember/tree/main/days/day-01-hello-world/firmware).
The two board initialization functions must run before display startup.

```c
pmu_init();
panel_reset_release();
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

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-01-hello-world/firmware
idf.py build
idf.py size
idf.py -p PORT flash monitor
```

Replace `PORT` with the board’s serial port.
Exit the monitor with `Ctrl+]`.
To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
The output is `build/merged-binary.bin`.

Keep `pmu_init()` and `panel_reset_release()` before display startup when changing the UI.

## If it goes wrong

| Symptom | Check / fix |
| --- | --- |
| Board freezes or disappears from USB after a few minutes | Keep `pmu_init()` before display startup. This project sets the AXP2101 USB input limit to 900 mA and charging to 300 mA. |
| Panel turns black while display calls return success | Keep `panel_reset_release()` before `bsp_display_start()`. V2 panel reset uses the TCA9554 expander. |
| Label is missing near the top-left | Center it so it clears the panel’s rounded corner. |

These settings belong to this V2 board and its power setup.

## Take it with you

Board startup is part of the program.
The PMU and reset fixes keep this V2 panel alive; an API success code cannot prove that the screen is visible.

[Read the build story](https://esptember.com/day/day-01-hello-world/story/) for the investigation and lessons behind this version.
