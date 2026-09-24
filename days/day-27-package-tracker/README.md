---
board: waveshare-amoled-18-v2
day: 27
title: Package Tracker
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-27-package-tracker.bin
summary: "A status board for everything in the mail — demoable in two minutes with EasyPost's mock packages, no real parcel required."
verification: "Boot, portal, store, and console verified; sandbox run pending a test key"
---

## The result

A status board for everything in the mail: one row per package, latest event line, color by state — orange in transit, cyan out for delivery, green delivered, red exception.
It's built against [EasyPost](https://easypost.com), chosen for one reason above all: **the free sandbox is a complete demo in a box.**
Test keys cost nothing forever, and mock tracking numbers `EZ1000000001` through `EZ1000000007` simulate every delivery state — you can fill the board with packages in seven different conditions in two minutes, indoors, with nothing in the mail.
This is the series' first **keyed** API, and the key is provisioned like every secret this month: over serial, into NVS, never into the repository.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**.
- **Connection:** a USB data cable; a phone for the Wi-Fi portal.
- **An API key:** a free [EasyPost](https://easypost.com) account — the **test** key (`EZTK...`) is all this lesson needs.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-27-package-tracker.bin](https://esptember.com/firmware/day-27-package-tracker.bin) and flash it at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-27-package-tracker.bin
```

Wi-Fi arrives through day 22's portal (join `esptember-setup`).
Then, over serial at 115200:

```
key EZTK_YOUR_TEST_KEY
add EZ1000000001
add EZ1000000004
add EZ1000000006
```

`add` takes up to six numbers (a second word names a carrier: `add 1Z... UPS`); `del` removes one; `refresh` fetches immediately.
Everything persists — key, numbers, tracker handles, and last-known statuses survive power loss.
With a production key, real tracking numbers work the same way.

## How it works

EasyPost's contract is create-then-read: a tracking number becomes a **tracker** object on first registration, and the tracker's id is the handle for every later query.
The firmware keeps that id per package and registers stragglers before each refresh.

Both the create and the read return the same tracker shape, so one function absorbs either:

```c
static void absorb_tracker(package_t *p, const cJSON *tracker)
```

That symmetry is what keeps the provider layer thin — and the thinness is deliberate, because this layer is the part you might swap (see below).

Auth is HTTP Basic with the key as username and an empty password; the refresh interval is fifteen minutes (packages move on truck time), with `refresh` on serial for the moment after the doorbell.

## Migrate it to another service

EasyPost is the teaching choice, not a marriage.
The provider layer is three functions — `api_request`, `absorb_tracker`, `register_new_numbers`/`refresh` — and porting them is a well-shaped job for an agent or an afternoon.
Alternatives we evaluated:

- **[17TRACK API](https://api.17track.net/en/doc)** — any-carrier from a bare number, free tier, key in a `17token` header, register-then-query like EasyPost. The strongest production choice.
- **[AfterShip](https://www.aftership.com/docs/tracking/quickstart)** — similar shape, generous docs, free tier.
- **[WhereParcel](https://whereparcel.com)** — newer entrant with a promotional free tier; simple endpoints, but weigh the longevity of a young service before building a daily driver on it.

A migration prompt that carries the constraints this firmware already honors:

> Port `days/day-27-package-tracker/firmware/main/main.c` from EasyPost to `<SERVICE>`. Keep everything outside the provider layer untouched: the NVS store, the serial grammar (`key`/`add`/`del`/`refresh`), the LVGL status board, and the 15-minute refresh. Replace `api_request` (auth: consult the service's docs — header token vs basic auth), `register_new_numbers` (if the service requires registration before queries, keep the per-package `registered`/handle fields; if not, delete them), and `absorb_tracker` (map the service's status vocabulary onto the existing `status_color` cases: delivered / out_for_delivery / in_transit / failure / pre_transit). Parse with cJSON against the existing 8 KB body buffer; if responses can exceed it, filter fields at the API or fail with a named error on `status_line`. Verify with the service's sandbox numbers if it has them, or a real tracking number if not, and confirm `D26_PACKAGE` serial lines carry status + latest event.

## Check the result

- No key: the board says so and names the serial command. Same for zero packages.
- With a test key, the three mock numbers above fill the board in one refresh: one **pre-transit** gray, one **out for delivery** cyan, one **delivered** green, each with a simulated event line.
- All seven mock numbers (`EZ1000000001`–`EZ1000000007`) exercise every color the board knows, including red.
- Power-cycle: the board returns with last-known statuses before the first fetch lands.

**Recorded evidence · September 22, 2026:** Boot, the portal handoff, the NVS store, and the console grammar were verified over serial. The sandbox run awaits a test key on the bench, noted in NOTES.md.

## Used resources

- [EasyPost Trackers API](https://docs.easypost.com/docs/trackers) — create-then-read, test mode with mock tracking numbers.
- Day 22's portal and the secrets-in-NVS rule, gaining its first API-key tenant.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-27-package-tracker/firmware
idf.py build
idf.py -p PORT flash monitor
```

To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
Keep `pmu_init()` and `panel_reset_release()` before display startup when changing the UI.
Webhooks are the natural v2 — EasyPost pushes tracker updates, which turns the status board into a doorbell.
