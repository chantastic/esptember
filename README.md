# ESPtember

30 days of individually flashable projects for the
[Waveshare ESP32-S3-Touch-AMOLED-1.8](https://www.waveshare.com/esp32-s3-touch-amoled-1.8.htm).

Live at [esptember.com](https://esptember.com).

## Structure

```
days/
  day-01-hello-world/
    README.md        # frontmatter + writeup — doubles as the day's web page
    firmware/        # that day's firmware project (toolchain varies by day)
src/                 # Astro site; renders days/*/README.md
public/firmware/     # collected merged .bin files, served by the site
```

Each day's `README.md` is the single source of truth: GitHub renders it in
the repo, and the Astro site renders it at `esptember.com/day/<slug>/`.

### Day frontmatter

```yaml
day: 1 # sort order
title: Hello World
toolchain: ESP-IDF v5.5 # noted on every lesson — toolchains vary by day
firmware: /firmware/day-01-hello-world.bin # optional, hosted merged binary
video: https://youtu.be/VIDEO_ID # optional, embedded when present
```

## Develop

```sh
pnpm install
pnpm dev
```

## Build a day's firmware

```sh
cd days/day-01-hello-world/firmware
idf.py set-target esp32s3
idf.py build merge-bin   # merge-bin produces build/merged-binary.bin
```

`pnpm build` runs `scripts/collect-firmware.sh`, which copies each day's
`build/merged-binary.bin` to `public/firmware/<day>.bin` before Astro builds.

## Deploy

```sh
pnpm run deploy
```

Deploys `dist/` to Cloudflare Workers (static assets) with custom domains
`esptember.com` and `www.esptember.com` (configured in `wrangler.jsonc`;
the zone must be on the same Cloudflare account).
