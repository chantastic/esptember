---
board: waveshare-amoled-18-v2
day: 33
title: Pipes
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL) / Cloudflare Workers
firmware: /firmware/day-33-pipes.bin
summary: "The resident identity does work: your connected accounts, live on the device, through a gateway that keeps every secret at the edge."
verification: "Verified on hardware: silent sign-in, seven providers listed, six live connections shared from the web"
---

## The result

Day 32's resident identity gets a job.
The board signs in silently, asks a ~100-line Cloudflare Worker for one screen's worth of data, and displays your **WorkOS Pipes** providers — GitHub, LinkedIn, YouTube, X, and friends — each marked live: green with a detail line if your account is connected, gray with a **scan-to-connect QR** if it isn't.
Connections made on the web appear on the device and vice versa, because they belong to your *user*, not to any app.
This day has a firmware image and a deployed Worker.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**.
- **A WorkOS environment** with AuthKit (device grant enabled) and Pipes providers configured, plus its secret API key for the Worker.
- **A Cloudflare account** for the gateway (`wrangler deploy` + one secret).
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated.

The published binary points at the series' gateway; self-hosters deploy their own and change one constant.

## Run it

Deploy your gateway:

```sh
cd days/day-33-pipes/gateway
npm install && npx wrangler deploy
npx wrangler secret put WORKOS_API_KEY
```

Point `GATEWAY` in the firmware at your Worker URL, build, and flash (or use the published image against the series' gateway):

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-33-pipes.bin
```

Provision once — portal for Wi-Fi, `client client_01...` over serial, one sign-in QR — and the provider list appears.
Every boot after is silent.

## How it works

The architecture is one sentence: **the device proves who it acts for; the Worker holds the power; WorkOS holds the provider tokens.**

The firmware is deliberately dumb — one authenticated request, one render:

```c
    cJSON *r = http_json(GATEWAY "/screen", "GET", NULL, NULL, access_token,
                         &status);
```

The Worker does all the composing.
It verifies the board's JWT against the AuthKit JWKS, extracts the user *and organization* from the claims, and calls the per-user provider listing — connections are workspace-scoped, and the org id matters:

```js
    const qs = new URLSearchParams({ supports_multiple_connections: "true" });
    if (orgId) qs.set("organization_id", orgId);
```

For every unconnected provider it pre-mints an authorization URL (the device just QRs it); for connected ones it can fetch a one-line detail through **Pipes Relay** — where the provider's token never exists on the device, in the Worker, or anywhere else it could leak.
Add an entry to the Worker's `DETAILS` table and redeploy: the device learns a new trick with no reflash.
Server-driven UI, ESP32 edition.

Tapping a gray row shows the QR; approve on your phone and the row flips green within one poll.
The list also refreshes itself every minute, so connections made **on the web** appear on the device unprompted.

## What went wrong

Three bugs, each instructive:

- **The 512-byte ambush:** esp_http_client's default header buffer couldn't hold a ~1 KB JWT bearer; the mangled request died as a connection reset that looked like a TLS mystery. `buffer_size_tx` is the cure.
- **The wrong endpoint:** listing `/data-integrations` returns the catalog, never the user's connections. The per-user truth lives at `/user_management/users/{id}/data_providers` — found not in the docs but in the production code of a prior project. Prior art beats documentation.
- **The one-word filter:** the connection state is the string `connected`, and the filter accepted every plausible synonym except that one. Six live connections sat invisible behind an `includes()`.

## Check the result

- After the one-time setup: **PIPES**, then your providers — connected ones green with a detail, unconnected gray with a `+`.
- Connections made at your identity site appear on the board (same user, same environment — shared by design).
- Tap a gray provider: QR → phone approval → the row flips green within seconds.

**Recorded evidence · September 22, 2026:** Verified live on hardware against a real WorkOS environment: silent re-auth, seven providers listed, six showing connected — the user's web-made connections shared to the device exactly as the model promises (`D31_PROVIDER ... connected=1`). Relay-fetched detail lines await the environment's Relay early-access enablement; the gateway degrades gracefully until then.

## Used resources

- [WorkOS Pipes](https://workos.com/docs/pipes) — providers, connected accounts, authorization URLs, and [Relay](https://workos.com/docs/pipes/relay) (early access): the forward proxy where provider tokens never reach your runtime.
- [jose](https://github.com/panva/jose) for JWKS verification at the edge.
- Days 21, 24, 30, 31: the portal, the Worker deployment pattern, the device grant, and the resident session.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

Gateway: `days/day-33-pipes/gateway` (wrangler).
Firmware: `days/day-33-pipes/firmware` (`idf.py build`), `GATEWAY` constant at the top of `main.c`.
The graduated version of this architecture — a dedicated identity service, operation-scoped provider access, rate limits, receipts — is what a production device fleet grows into; this Worker is its teaching-sized seed.
