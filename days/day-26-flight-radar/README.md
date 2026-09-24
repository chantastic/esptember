---
board: m5stack-stopwatch
day: 26
title: Flight Radar
toolchain: Arduino CLI (esp32 core) + M5Unified + ArduinoJson
firmware: /firmware/day-26-flight-radar.bin
summary: "A radar scope on your wrist: every aircraft within 25 nautical miles, from a keyless community API."
verification: "Boot verified; endpoint host-verified live; on-device fetch pending Wi-Fi credentials"
---

## The result

The round face becomes a radar scope: you at the center, dim green range rings, a rotating sweep, and every aircraft within 25 nautical miles as a blip with callsign, flight level, and a velocity leader showing where it's headed.
The data comes from [adsb.lol](https://adsb.lol) — a community aggregator of hobbyist ADS-B receivers, free and keyless.
Planes broadcast their position unencrypted at 1090 MHz; volunteers with $20 dongles catch it; this board just asks nicely every eight seconds.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** M5Stack Stopwatch Dev Kit (ESP32-S3, C152): Wi-Fi, 1.75″ round AMOLED, battery.
- **Connection:** a USB data cable for flashing and one-time provisioning.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [arduino-cli](https://arduino.github.io/arduino-cli/) with the esp32 core, M5Unified, and ArduinoJson.

The display is 466 × 466 pixels behind a circular aperture.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-26-flight-radar.bin](https://esptember.com/firmware/day-26-flight-radar.bin) and flash it at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-26-flight-radar.bin
```

On first boot the board hosts the `esptember-setup` portal — join it from a phone and fill the form.
Credentials are *shared*: a board provisioned for any lesson is provisioned for radar.
Serial remains available for coordinates (default: Portland, OR):

```
loc 45.52 -122.68
```

Set `loc` to your coordinates; the scope centers on you.
Holding **both pushers for two seconds** forgets Wi-Fi and reopens the portal — reconfiguration never needs a computer.

## How it works

One request returns every aircraft near a point:

```c
  snprintf(url, sizeof(url), "https://api.adsb.lol/v2/point/%.4f/%.4f/%d",
           lat, lon, RADIUS_NM);
```

An airliner's full ADS-B record is large, so the parse is **filtered**: ArduinoJson's filter document keeps only the five fields the scope draws — callsign, position, altitude, track — and discards the rest in flight.
The same streaming discipline as day 23, upgraded for payloads that stop being polite.

Geo math at 25 nm needs no globe: an equirectangular projection — one cosine for longitude shrinkage — converts degrees to nautical miles to pixels.
Blips carry a velocity leader (a short line along the aircraft's track) because a scope without leaders shows where planes *were*.

The sweep rotates once per fetch interval and is, technically, cosmetic.
It is also entirely mandatory.

## Check the result

- Unprovisioned: the screen shows **SETUP** and hosts the `esptember-setup` portal.
- Provisioned near any city: blips with callsigns appear inside the rings within seconds, headers count aircraft, `D25_AIRCRAFT` logs each fetch.
- Blips crawl believably: an airliner at altitude crosses the 25 nm scope in a few minutes, leaders pointing along its path.
- B forces a refetch; `loc` teleports the scope anywhere (try an airport).

**Recorded evidence · September 22, 2026:** The firmware boot and provisioning states were verified over serial; the exact endpoint the firmware calls was verified live from the host during development (12 aircraft within 25 nm of the default coordinates at time of check). The on-device fetch awaits Wi-Fi credentials on the bench, noted in NOTES.md.

## Used resources

- [adsb.lol](https://api.adsb.lol) — open, keyless community ADS-B aggregation (`/v2/point/lat/lon/radius`).
- ADS-B itself: aircraft transponders broadcasting position in the clear at 1090 MHz, aggregated by volunteers worldwide.
- [ArduinoJson filtering](https://arduinojson.org/v7/api/json/deserializejson/) — parse five fields, skip the rest.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

Install the toolchain pieces (once):

```sh
arduino-cli core install esp32:esp32
arduino-cli lib install M5Unified ArduinoJson
```

Build and flash from the repository root:

```sh
cd days/day-26-flight-radar
./scripts/build.sh
./scripts/flash.sh PORT
```

The merged image lands in `.build/firmware/flight_radar.ino.merged.bin`.
`RADIUS_NM` scales the scope; a tap-to-inspect detail view and squawk-code coloring are the natural extensions.
