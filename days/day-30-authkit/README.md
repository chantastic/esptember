---
board: waveshare-amoled-18-v2
day: 30
title: WorkOS AuthKit
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-30-authkit.bin
summary: "The finale: a keyboardless device signs a real user into a real identity provider — the OAuth device flow, on a wrist-sized screen."
verification: "Boot, portal, and flow states verified; live sign-in pending Wi-Fi credentials and a client id"
---

## The result

The finale: a device with no keyboard and no browser signs a real user into a real identity provider.
The board asks WorkOS for a code pair, shows a QR of the verification URL and the human-readable code beneath it, and polls until you approve on your phone — then greets you by email address.
This is the OAuth 2.0 Device Authorization Grant (RFC 8628), the flow your TV apps use, running on a wrist-sized AMOLED.
No secrets ship in this firmware: the client ID is public by design, entered once over serial; Wi-Fi arrives through day 21's portal.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**.
- **Connection:** a USB data cable, a phone for the setup + approval, and a free [WorkOS](https://workos.com) account with AuthKit and the device grant enabled (the dashboard gives you a `client_01...` id).
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-30-authkit.bin](https://esptember.com/firmware/day-30-authkit.bin) and flash it at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-30-authkit.bin
```

Two one-time setup steps:
Wi-Fi via the captive portal (join `esptember-setup`, fill the form), then your public client id over serial:

```
client client_01XXXXXXXXXXXXXXXXXXXXXXXXX
```

The board reboots into the flow: QR, code, approve on your phone, greeted by name.

## How it works

The device flow is two HTTP calls and patience.

First, the board trades its client id for a code pair:

```c
#define AUTHORIZE_URL "https://api.workos.com/user_management/authorize/device"
```

The response carries a `user_code` for human eyes, a `verification_uri_complete` for the QR (LVGL renders it with one widget), a `device_code` the board keeps private, and a polling `interval`.

Then the board polls the token endpoint with the device-code grant:

```c
#define GRANT_TYPE "urn:ietf:params:oauth:grant-type:device_code"
```

Three answers are possible, and handling all three is the lesson: `authorization_pending` (keep waiting), `slow_down` (RFC 8628's speeding ticket — the board adds five seconds and behaves), and `200` with a user object (signed in).
Codes expire; an expired pair is thrown away and a fresh one fetched, new QR and all.

The security shape is worth staring at: the *user code* travels through the human, the *device code* never leaves the board, the client id is public, and no password ever exists anywhere near this firmware.
The phone does the authenticating on WorkOS's pages; the board only ever learns the verdict.

## Check the result

- Fresh flash: the portal, then the client-id prompt, each state named on screen.
- With both set: **SIGN IN** with a scannable QR and an eight-character code.
- Scanning opens AuthKit on your phone; approving flips the board to **AUTHORIZED — signed in as you@example.com** within one poll interval.
- Declining shows **DENIED**; letting the code expire fetches a fresh pair automatically.

**Recorded evidence · September 22, 2026:** Boot, the portal handoff, the client-id gate, and the flow's state machine were verified over serial (`D30_MODE`, `D30_STATUS`); the endpoints and grant are the ones proven in this hardware family by a prior project. The live sign-in awaits bench Wi-Fi and a client id, noted in NOTES.md.

## Used resources

- [RFC 8628](https://datatracker.ietf.org/doc/html/rfc8628) — OAuth 2.0 Device Authorization Grant, including the `slow_down` contract.
- [WorkOS AuthKit](https://workos.com/docs) user management endpoints: `authorize/device` and `authenticate`.
- Day 21's portal (lifted verbatim) and LVGL's QR widget.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-30-authkit/firmware
idf.py build
idf.py -p PORT flash monitor
```

To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
Keep `pmu_init()` and `panel_reset_release()` before display startup when changing the UI.
The natural sequel is what the sign-in *unlocks*: the response carries tokens, and every service day in this series is one `Authorization: Bearer` header away from being per-user.
