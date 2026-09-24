---
board: waveshare-amoled-18-v2
day: 22
title: Captive Portal
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-22-captive-portal.bin
summary: "No credentials in the repo, ever: the board becomes its own setup page. A liar's DNS and an HTTP form."
verification: "Portal AP + DNS + HTTP verified over serial; phone-join flow pending hands-on"
---

## The result

The gateway lesson: every internet-facing day after this one assumes the board can get online **without credentials in the repository**.
No saved Wi-Fi → the board becomes an open access point named `esptember-setup`; join it from a phone and the setup page opens by itself, listing the networks the board can see.
Pick one, type the password, and the board saves it to NVS and reboots onto your network — showing its SSID, IP, and signal strength on screen.
Hold **BOOT** during power-up to forget everything and start over.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**: ESP32-S3, CO5300 panel, 16 MB flash, 8 MB PSRAM.
- **Connection:** a USB data cable, plus any phone or laptop for the setup flow.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below. ESP-IDF is not needed.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-22-captive-portal.bin](https://esptember.com/firmware/day-22-captive-portal.bin) and open a terminal in the download directory.
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
  write-flash 0x0 day-22-captive-portal.bin
```

When flashing completes, the screen says **SETUP** and the network `esptember-setup` appears.

## How it works

The trick that makes phones pop the setup page automatically is a **DNS server that lies**: it answers every query with the board's own address.

```c
        buf[p++] = 192; buf[p++] = 168; buf[p++] = 4; buf[p++] = 1;
```

A phone joining any network probes a known URL to test connectivity.
The probe's DNS lookup lands on the liar, the HTTP request reaches the board instead of the internet, the phone sees an unexpected page — and offers it as a captive portal.
That's the entire mechanism; the rest is a 404 handler that redirects strays to the form.

The board runs **AP+STA** mode so the portal page can scan for networks while hosting one — the dropdown you pick from is a live scan taken as the page loads.

The form posts back, credentials go to NVS, the board reboots and becomes a normal station.
Reconnect logic retries forever, and the **BOOT-hold escape hatch** exists precisely because "forever" is the wrong answer to a typo'd password.

Later days reuse this as a library: `portal.c` exposes load/save/clear and the portal itself, and NVS is the only place secrets ever live.

## Check the result

- Fresh flash: the screen reads **SETUP**, and `esptember-setup` shows up in your phone's Wi-Fi list.
- Joining it pops the portal automatically (or browse to `192.168.4.1`), with your real networks in the dropdown.
- Submitting reboots the board; the screen walks CONNECTING → **ONLINE** with SSID, IP, and RSSI.
- Power-cycling keeps the connection — the credentials survived.
- Holding BOOT while plugging in returns to SETUP with the slate wiped.

**Recorded evidence · September 22, 2026:** Portal mode, the DNS responder, and the HTTP server were verified running over serial (`D21_PORTAL up`). The phone-join, form-submit, and station handoff await a hands-on pass, noted in NOTES.md.

## Used resources

- The captive-portal detection dance: OS connectivity probes (`captive.apple.com` and friends) + DNS interception, as documented across RFC 8910 and a decade of hotspot lore.
- [WiFiManager](https://github.com/tzapu/WiFiManager) — the Arduino-world prior art this lesson builds from scratch instead of wrapping.
- ESP-IDF `esp_http_server`, `esp_wifi` AP+STA mode, and NVS.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-22-captive-portal/firmware
idf.py build
idf.py -p PORT flash monitor
```

To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
The output is `build/merged-binary.bin`.
Keep `pmu_init()` and `panel_reset_release()` before display startup when changing the UI.
`portal.c` is deliberately reusable — later days lift it whole. Adding an API-key field to the form is one input and one NVS write.
