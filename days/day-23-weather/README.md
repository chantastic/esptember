---
board: m5stack-stopwatch
day: 23
title: Weather Station
toolchain: Arduino CLI (esp32 core) + M5Unified + ArduinoJson
firmware: /firmware/day-23-weather.bin
summary: "The board's first fetch: Open-Meteo, no key, no account — temperature, conditions, and a 12-hour arc around the rim."
verification: "Boot, provisioning states, and render verified; live fetch pending Wi-Fi credentials"
---

## The result

The portal's payoff: the board is online, and the sky is the first thing worth fetching.
[Open-Meteo](https://open-meteo.com/) serves current conditions and forecasts with **no API key, no account, no strings** — pure JSON over HTTPS.
The round face becomes a watch complication: big temperature, condition text, high/low, wind, and a 12-hour temperature arc dotted around the rim like a bezel.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** M5Stack Stopwatch Dev Kit (ESP32-S3, C152): Wi-Fi, 1.75″ round AMOLED, battery.
- **Connection:** a USB data cable for flashing and one-time Wi-Fi provisioning.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [arduino-cli](https://arduino.github.io/arduino-cli/) with the esp32 core, M5Unified, and ArduinoJson.

The display is 466 × 466 pixels behind a circular aperture.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-23-weather.bin](https://esptember.com/firmware/day-23-weather.bin) and open a terminal in the download directory.
This is a merged image containing the bootloader, partition table, and application.

Find your serial port:

```sh
# macOS
ls /dev/cu.usbmodem*
# Linux
ls /dev/ttyACM*
```

Flash the image at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-23-weather.bin
```

On first boot the screen says **SETUP**: join the Wi-Fi network `esptember-setup` from your phone and the setup page opens itself — pick your network, enter the password, done (day 22's portal, ported to this board).
Serial remains the power-user path at 115200:

```
wifi YOUR_SSID YOUR_PASSWORD
loc 45.52 -122.68
```

Credentials persist in NVS; `forget` erases them.
Holding **both pushers for two seconds** forgets Wi-Fi and reopens the portal — reconfiguration never needs a computer.
The `loc` line sets your coordinates (the default is Portland, OR).

## How it works

One URL carries the whole lesson — current conditions, hourly temperatures, and the daily range, scoped to twelve hours:

```c
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude="
           "%.4f&current=temperature_2m,weather_code,wind_speed_10m"
           "&hourly=temperature_2m&daily=temperature_2m_max,temperature_2m_min"
           "&forecast_hours=12&forecast_days=1&timezone=auto",
```

The response parses straight off the HTTP stream with ArduinoJson — no buffer sized to the whole payload — and conditions arrive as WMO interpretation codes, decoded by a ladder of thresholds into words (`0` clear, `61`–`67` rain, `95`+ thunderstorm).

The rim arc is the round display earning its shape: twelve dots sweep 270°, one per hour, radius and warmth riding the temperature between the half-day's min and max.
A glance reads the trend — dots climbing toward the top-right means bring no jacket.

Fetches happen on connect and every ten minutes after; **B** forces a refresh.
Ten minutes is deliberate politeness — Open-Meteo is free, and free APIs stay free when clients behave.

## Check the result

- Unprovisioned: the screen shows **SETUP** and hosts the `esptember-setup` portal.
- After `wifi ...`: reboot, join, and the first fetch fills the face — temperature large, condition text, H/L, wind, twelve rim dots.
- `D22_WEATHER` lines on serial log each fetch's parsed values.
- B refetches immediately; `loc` moves the station anywhere on Earth.

**Recorded evidence · September 22, 2026:** Boot, the unprovisioned state, serial provisioning handlers, and rendering were verified on the installed firmware. The live fetch awaits Wi-Fi credentials on the bench, noted in NOTES.md.

## Used resources

- [Open-Meteo](https://open-meteo.com/) — genuinely keyless weather API; WMO weather interpretation codes.
- [ArduinoJson](https://arduinojson.org/) stream parsing.
- Day 22's rule, adapted: secrets in NVS only — this board's portal is a serial line instead of a web form.

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
cd days/day-23-weather
./scripts/build.sh
./scripts/flash.sh PORT
```

The merged image lands in `.build/firmware/weather.ino.merged.bin`.
Obvious extensions: a Fahrenheit toggle, precipitation probability on the rim's second ring, and pairing with an ENV sensor for inside-vs-outside.
