---
board: waveshare-amoled-18-v2
day: 7
title: LVGL Basics
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-07-lvgl-basics.bin
summary: "Buttons, a slider, a switch, and a second screen. The display becomes an interface."
verification: "Injected-touch harness PASS; physical touch calibration pending"
---

## The result

Days 01–06 used LVGL as a renderer: put a thing on screen, leave it there.
Today it becomes a UI toolkit.
The firmware shows a control panel — a counting button, a brightness slider wired to the panel's real brightness command, a switch that floods the screen ESPtember orange, and a second screen that slides in and out without losing any state.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**: ESP32-S3, CO5300 panel, CST816-family touch, 16 MB flash, 8 MB PSRAM.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below. ESP-IDF is not needed.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated. Dependencies are declared in the firmware project.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-07-lvgl-basics.bin](https://esptember.com/firmware/day-07-lvgl-basics.bin) and open a terminal in the download directory.
This is a merged image containing the bootloader, partition table, and application.

Find your serial port:

```sh
# macOS
ls /dev/cu.usbmodem*
# Linux
ls /dev/ttyACM*
```

On Windows, use the board's COM port from Device Manager.
Replace `PORT` with your port, then flash the image at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-07-lvgl-basics.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## How it works

Three ideas carry the whole library: widgets are a tree, events connect touch to code, and screens are just root objects you can swap.

Every interactive widget works the same way: create it, attach a callback for the event you care about.

```c
static void count_button_cb(lv_event_t *e)
{
    lv_obj_t *label = lv_event_get_user_data(e);
    lv_label_set_text_fmt(label, "pressed %d", ++day07_press_count);
}
```

The slider drives real hardware — the panel's brightness command — and `LV_EVENT_VALUE_CHANGED` fires continuously while dragging, so the panel tracks your finger:

```c
static void brightness_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    day07_brightness = lv_slider_get_value(slider);
    bsp_display_brightness_set(day07_brightness);
}
```

The slider's floor is 10%, so the screen can never go fully dark with no way to see the slider that rescues it.

Layout is a flex column, not pixel math.
When the children outgrow the container, LVGL makes it scrollable — scrolling isn't a widget, it's what any overflowing container does.
Drag anywhere.

The second screen is one call:

```c
    lv_screen_load_anim(about_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0,
                        false);
```

Both screens are built once at startup and kept in memory.
Come back from About and the counter, slider, and switch are exactly where you left them — a screen swap changes what's drawn, not what exists.

## The proof

This day introduces a verification harness in the day-12 tradition: the firmware answers `status`, `tap`, `drag`, and `capture` over USB, injecting gestures through a virtual LVGL pointer device and dumping real screen snapshots back.
A scripted run pressed the button, dragged the slider, toggled the switch, scrolled to About, and navigated both directions — every state change confirmed against the firmware's own readback, every screen confirmed by captured image.
LVGL cannot tell the virtual finger from a real one: same hit-testing, same events, same animations.

The physical touch layer has one open item: coordinate calibration against the CST816 is still being verified on hardware.
The harness command `touchlog on` streams raw controller coordinates for that check.

## Check the result

- The control panel appears: title, Count button, "pressed 0", Brightness slider, Orange mode switch.
- Tapping **Count** increments the label.
- Dragging **Brightness** visibly dims the panel, live.
- **Orange mode** floods the background orange; off returns to black.
- Drag up to reveal **About**; the screen slides left. **Back** slides home with all state intact.

**Recorded evidence · September 21, 2026:** A scripted harness run injected every gesture above through the firmware's virtual pointer device and confirmed each state change via readback and captured screenshots (`scripts/check-device.py`, result PASS). Physical touch calibration on the CST816 is recorded as pending in NOTES.md.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-07-lvgl-basics/firmware
idf.py build
idf.py -p PORT flash monitor
```

Exit the monitor with `Ctrl+]`.
To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
The output is `build/merged-binary.bin`.
Keep `pmu_init()` and `panel_reset_release()` before display startup when changing the UI.
