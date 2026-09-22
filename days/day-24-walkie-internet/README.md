---
board: m5stack-stopwatch
day: 24
title: Walkie-Talkie, Worldwide
toolchain: Arduino CLI + M5Unified / ESP-IDF v5.5 + Waveshare BSP / Cloudflare Workers
firmware: /firmware/day-24-walkie-internet.bin
summary: "Same walkie, new radio: the transport swaps from ESP-NOW to a Cloudflare relay, and the range becomes Earth."
verification: "Relay fan-out host-verified; both halves boot-verified; over-the-internet audio pending Wi-Fi credentials"
---

## The result

Day 23 with one organ transplanted: the transport.
The mic, the PTT crown, the 15 ms frames, the 22-channel dial, the jitter buffer — identical.
But frames now ride a WebSocket to a ~60-line Cloudflare Worker, where one Durable Object per channel fans them out to every listener on that channel, anywhere on Earth.
Same UI, new radio — the whole lesson is how little else had to change.
This day has two firmware images and one deployed Worker.

## What you need

- **Boards:** the M5 StopWatch (transmitter) and the Waveshare AMOLED 1.8 (receiver), each on any Wi-Fi network — different networks is the point.
- **Connection:** USB data cables for flashing and one-time provisioning.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** arduino-cli + M5Unified + WebSockets (links2004) for TX; ESP-IDF v5.5 for RX; wrangler for your own relay.

The published binaries point at the series' relay; self-hosters redeploy `relay/` and change one hostname.

## Run it

Download both images:
[day-24-walkie-internet.bin](https://esptember.com/firmware/day-24-walkie-internet.bin) (StopWatch TX) and
[day-24-walkie-internet-waveshare.bin](https://esptember.com/firmware/day-24-walkie-internet-waveshare.bin) (AMOLED 1.8 RX).

Flash each at `0x0`:

```sh
uvx esptool --chip esp32s3 --port STOPWATCH_PORT \
  write-flash 0x0 day-24-walkie-internet.bin
uvx esptool --chip esp32s3 --port WAVESHARE_PORT \
  write-flash 0x0 day-24-walkie-internet-waveshare.bin
```

Provision Wi-Fi: **both boards run the captive portal** — each hosts `esptember-setup` when unprovisioned; join from a phone and fill the form.
Match channels, hold the crown, talk across the world.
On the StopWatch, holding **both pushers for two seconds** forgets Wi-Fi and reopens the portal.

## How it works

The relay is the entire backend, and it fits in a comment:

```js
    server.addEventListener("message", (event) => {
      // Voice frames are binary; forward to everyone but the speaker.
      for (const socket of this.sockets)
        if (socket !== server && socket.readyState === 1)
          socket.send(event.data);
    });
```

A Durable Object is a tiny stateful server that Cloudflare conjures per name: `/channel/7` routes to the object named "7", which holds channel 7's sockets.
Twenty-two channels by convention, infinity by implementation — rooms are URLs.

On the boards, the day-23 frame travels unchanged; only the `esp_now_send` became `sendBIN`, and the receive callback became a WebSocket event.
The squelch byte survives as defense-in-depth (the room *is* the channel now), the jitter buffer is untouched — internet jitter and radio jitter are the same disease, and 60 ms of prebuffer treats both.
Changing channels is rejoining a different URL.

Provisioning shows the day-21 rule paying off: the RX lifts `portal.c` verbatim, the TX reuses day 22's NVS namespace — provision once, every lesson benefits.

## Check the result

- Both boards show their link state climb: Wi-Fi → `relay linked`.
- Hold the crown: **ON AIR** on the TX, **RECEIVING** on the RX, voice from the speaker — with boards on the same desk or on different continents.
- Mismatched channels go quiet; matching them restores audio.
- The relay URL in a browser answers `esptember walkie relay`.

**Recorded evidence · September 22, 2026:** The deployed relay was verified from a host script — two WebSocket clients on channel 7, binary bytes fanned to the listener in 528 ms, no echo to the sender. Both firmware halves boot-verified into their provisioning states (`D24_STATUS`, `D21_PORTAL up`). Device-to-device audio over the relay awaits Wi-Fi credentials on the bench, noted in NOTES.md.

## Used resources

- [Cloudflare Durable Objects](https://developers.cloudflare.com/durable-objects/) — per-name stateful edge objects; the free tier runs this relay.
- [esp_websocket_client](https://components.espressif.com/components/espressif/esp_websocket_client) (ESP-IDF) and [WebSockets](https://github.com/Links2004/arduinoWebSockets) (Arduino).
- Days 21–23: the portal, the shared NVS namespace, and the entire audio pipeline, reused.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember/days/day-24-walkie-internet
```

Deploy your own relay (optional — one command):

```sh
cd relay && npx wrangler deploy
```

TX half: `./scripts/build-stopwatch.sh`, upload as in day 23.
RX half: `cd firmware/waveshare && idf.py build flash`.
Point both at your relay by changing `RELAY_HOST` / `RELAY_URI_FMT`.

Day 23 ended promising this lesson; the pair is the series' clearest argument that **transports are swappable when the frame is the contract**.
