---
day: 1
title: Hello World
toolchain: ESP-IDF v5.5
firmware: /firmware/day-01-hello-world.bin
---

The smallest possible program for the Waveshare ESP32-S3-Touch-AMOLED-1.8:
print `Hello, ESPtember!` over USB once a second. No display, no Wi-Fi,
no framework overhead — the whole flashable image is ~220KB.

```c
void app_main(void)
{
    while (true) {
        printf("Hello, ESPtember!\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

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

4. Watch it say hello:

```sh
screen /dev/cu.usbmodem1101 115200
```

(exit `screen` with `ctrl-a` then `k`, then `y`)

## Build from source

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) v5.5+.

```sh
cd firmware
idf.py set-target esp32s3
idf.py -p /dev/cu.usbmodem1101 flash monitor
```

(exit the monitor with `ctrl-]`)

## What's in the image

| offset  | file                | what                          |
| ------- | ------------------- | ----------------------------- |
| 0x0     | bootloader.bin      | second-stage bootloader       |
| 0x8000  | partition-table.bin | where the app lives in flash  |
| 0x10000 | app                 | the ~159KB hello world binary |

The downloadable `.bin` is all three merged into one image flashed at
offset `0x0` — that's why the flash command is a single line.
