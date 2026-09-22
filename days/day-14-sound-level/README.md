---
board: waveshare-amoled-18-v2
day: 14
title: Sound Level (dB)
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL, esp_codec_dev)
firmware: /firmware/day-14-sound-level.bin
summary: "The onboard mic feeds an RMS meter: live dB readout, bar, and peak hold. Honestly uncalibrated."
verification: "Live room-tone readings observed over serial"
---

## The result

Day 08 sent audio out; today audio comes in.
The onboard microphone feeds an RMS meter: a big dB readout, a live bar, and a white peak-hold marker that rides the loudest moment for three seconds.
Honesty first: a MEMS mic with no calibration measures **dBFS** — decibels relative to the loudest sample the converter can represent — not absolute SPL.
The relative motion is real; the absolute number would need a reference meter.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**: ESP32-S3, CO5300 panel, ES8311 codec, onboard microphone, 16 MB flash, 8 MB PSRAM.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below. ESP-IDF is not needed.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated. Dependencies are declared in the firmware project.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-14-sound-level.bin](https://esptember.com/firmware/day-14-sound-level.bin) and open a terminal in the download directory.
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
  write-flash 0x0 day-14-sound-level.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## How it works

The mic path mirrors day 08's speaker path — same codec, opposite direction: init I2S, init the microphone codec, read PCM.
Metering happens in 50 ms windows: 800 samples per reading at 16 kHz, 20 readings per second.

RMS is the physically meaningful average for acoustic energy — peak would jump on every click, mean would cancel to zero:

```c
        double sum = 0;
        for (int i = 0; i < WINDOW_SAMPLES; i++)
            sum += (double)window[i] * window[i];
        float rms = sqrtf(sum / WINDOW_SAMPLES);
```

The decibel conversion pins 0 dB to a full-scale sample and clamps silence at -96, the theoretical floor of 16-bit audio:

```c
        float db = rms > 0.5f ? 20.0f * log10f(rms / 32768.0f) : -96.0f;
```

The peak marker holds the loudest reading for three seconds, then follows the level back down — the standard meter behavior that lets you catch a transient after it happens.

The mic runs on its own task; the UI reads shared values on a 50 ms LVGL timer matched to the metering rate.
Same separation as day 08: audio never runs on the UI task, in either direction.

## Check the result

- The meter shows a large dBFS number, a bar, and a white peak marker.
- A quiet room sits somewhere around -60 to -45 dBFS; the bar breathes with ambient noise.
- Speaking near the board jumps the number 20+ dB and the peak marker holds your loudest syllable for three seconds.
- A clap pins the peak marker well right of the live bar, then it decays.
- The footer says `relative, uncalibrated`, because it is.

**Recorded evidence · September 21, 2026:** Live readings observed over serial on the installed firmware — quiet-room tone around -54 dBFS with peak-hold tracking at -47 dBFS and the level responding to ambient sound.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-14-sound-level/firmware
idf.py build
idf.py -p PORT flash monitor
```

Exit the monitor with `Ctrl+]`.
To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
The output is `build/merged-binary.bin`.
Keep `pmu_init()` and `panel_reset_release()` before display startup when changing the UI.
To calibrate toward real SPL, measure a known source with a reference meter and add the offset — the code needs one constant.
