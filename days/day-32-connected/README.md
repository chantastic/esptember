---
board: waveshare-amoled-18-v2
day: 32
title: Connected
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-32-connected.bin
summary: "September's 31st: the sign-in becomes resident. One QR ever, silent renewal forever, and a session card read straight out of the JWT."
verification: "Silent NVS resume, local claims decode, and live renewal verified on hardware"
---

## The result

September doesn't have a 31st, but the series had unfinished business: day 31 signed in and kept nothing.
This day makes the identity **resident**.
The refresh token persists in NVS, so the board signs in silently on every boot — one QR scan, ever — and the screen becomes a session card: your email, organization, session id, and role, with a live countdown to token expiry… which the board quietly renews before it hits zero, forever.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**.
- **Connection:** a USB data cable; a phone for the one-time Wi-Fi portal and the one-time sign-in QR.
- **A WorkOS application** with the Device Authorization Grant enabled (day 31's setup); its public `client_01...` id.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-32-connected.bin](https://esptember.com/firmware/day-32-connected.bin) and flash it at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-32-connected.bin
```

First boot: Wi-Fi via the portal, `client client_01...` over serial, one QR sign-in.
Every boot after: the card appears by itself, captioned `resumed silently from NVS`.
`signout` over serial evicts the residency.

## How it works

Two ideas carry the day.

**Residency is one saved string.** The device grant's response includes a refresh token; persist it, and every boot trades it for a fresh session:

```c
    if (cJSON_IsString(rt)) save_str("authkit", "refresh", rt->valuestring);
```

WorkOS rotates the refresh token on every use — the comment in the code says the operational rule: always keep the newest.
The boot path tries the refresh first and only falls back to the QR ceremony when there's nothing to refresh.

**The token is a document, not a ticket stub.** A JWT is three base64url blobs, and the middle one is readable claims — who, which org, which session, what role, until when:

```c
static void decode_claims(void)
```

Decoding it locally costs no network call and no secrets; the session card is the token explaining itself.
AuthKit access tokens live about five minutes, so the countdown makes the renewal loop *visible*: watch it drain toward zero, then snap back with `renewed silently` — the OAuth refresh cycle, animated on a desk ornament.

## Check the result

- First boot walks portal → client id → QR → **CONNECTED**, with your email in green and org/session/role beneath.
- Power-cycle: the card returns unprompted, captioned `resumed silently from NVS`.
- The countdown drains; near 30 seconds it renews and the caption flips to `renewed silently`.
- `signout` over serial forgets the session; next boot shows the QR again.

**Recorded evidence · September 22, 2026:** Verified live on hardware: silent resume via refresh grant (`D31_REFRESH ok`) — notably across a *reflash*, since the token home survives in NVS — claims decoded on-device (`D31_CLAIMS org=… role=member`), and the card rendered with a ticking countdown.

## Used resources

- OAuth refresh-token rotation (WorkOS rotates per use — keep the newest).
- JWT structure: base64url header/claims/signature; claims are yours to read.
- Days 21 and 30: the portal and the device grant, unchanged underneath.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-32-connected/firmware
idf.py build
idf.py -p PORT flash monitor
```

To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
Keep `pmu_init()` and `panel_reset_release()` before display startup when changing the UI.
The sequel gives this resident identity something to do: day 33 turns it into your connected accounts, live on the same screen.
