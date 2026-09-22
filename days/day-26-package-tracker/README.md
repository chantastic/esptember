---
board: waveshare-amoled-18-v2
day: 26
title: Package Tracker
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-26-package-tracker.bin
summary: "The first keyed API: 17TRACK follows any carrier, the key lives in NVS, and the screen becomes a status board."
verification: "Boot, portal, store, and console verified; live tracking pending an API key"
---

## The result

A status board for everything in the mail: one row per package, latest event line, color by state — orange in transit, cyan out for delivery, green delivered, red exception.
The data comes from [17TRACK](https://api.17track.net), whose free tier follows any carrier from a bare tracking number.
This is the series' first **keyed** API, and the key is provisioned like every secret this month: over serial, into NVS, never into the repository.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**.
- **Connection:** a USB data cable; a phone for the Wi-Fi portal.
- **An API key:** a free [17TRACK API](https://api.17track.net) account (their free tier covers a personal mailbox comfortably).
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-26-package-tracker.bin](https://esptember.com/firmware/day-26-package-tracker.bin) and flash it at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-26-package-tracker.bin
```

Wi-Fi arrives through day 21's portal (join `esptember-setup`).
Then, over serial at 115200:

```
key YOUR_17TRACK_KEY
add 9400100000000000000000
```

`add` takes up to six numbers; `del` removes one; `refresh` fetches immediately.
Everything persists — key, numbers, and last-known statuses survive power loss.

## How it works

17TRACK's contract has one wrinkle worth teaching: numbers must be **registered** before they can be queried.
The firmware tracks a `registered` flag per package and registers stragglers before every fetch — idempotent, so a re-registration costs nothing.

The fetch itself is one POST with the key in a header and every number in the body:

```c
    esp_http_client_set_header(client, "17token", api_key);
```

The response nests deep — `data.accepted[].track_info.latest_status.status` — and the parse walks it defensively, keeping two strings per package: a machine status for the color map and a human event line for the row.

The refresh interval is fifteen minutes, the free-tier citizenship this series keeps practicing: packages move on truck time, not poll time.
`refresh` over serial exists for the impatient moment after the doorbell.

The key rides NVS alongside day 21's Wi-Fi credentials — same rule, one more tenant: **the published binary is identical for everyone; the device gets personalized, never the firmware.**

## Check the result

- No key: the board says so and names the serial command. Same for zero packages.
- With key + numbers: rows appear with statuses within one refresh, `D26_PACKAGE` lines logging each.
- A delivered package rows green; an in-transit one rows orange with its latest scan event underneath.
- Power-cycle: the board comes back with last-known statuses before the first fetch lands.

**Recorded evidence · September 22, 2026:** Boot, the portal handoff, the NVS store, and the console grammar (`key`/`add`/`del`/`refresh`) were verified over serial. Live tracking awaits an API key on the bench, noted in NOTES.md.

## Used resources

- [17TRACK API v2.2](https://api.17track.net/en/doc) — register + gettrackinfo, key in the `17token` header.
- Day 21's portal and the secrets-in-NVS rule, gaining its first API-key tenant.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-26-package-tracker/firmware
idf.py build
idf.py -p PORT flash monitor
```

To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
Keep `pmu_init()` and `panel_reset_release()` before display startup when changing the UI.
Webhooks are the natural v2 — 17TRACK can push instead of being polled, which turns the status board into a doorbell.
