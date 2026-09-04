# ESPtember

30 days of individually flashable projects for the
[Waveshare ESP32-S3-Touch-AMOLED-1.8](https://www.waveshare.com/esp32-s3-touch-amoled-1.8.htm).

Live at [esptember.com](https://esptember.com).

## Structure

```
days/
  day-01-hello-world/
    README.md        # practical guide: website + plain Markdown
    STORY.md         # build narrative: discoveries and lessons
    firmware/        # that day's firmware project (toolchain varies by day)
src/                 # Astro site; renders days/*/README.md
public/firmware/     # collected merged .bin files, served by the site
```

Each day's `README.md` is the single source of truth: GitHub renders it in
the repo, and the Astro site renders it at `esptember.com/day/<slug>/`.
The same body is served at `/day/<slug>/guide.md` for agents and offline reading.
`STORY.md` renders at `/day/<slug>/story/`; the guide owns current operating
instructions, while the story records the investigation.
The heading map comes from Astro’s rendered headings, with a sticky desktop
outline and a collapsible menu below 1050px.
`/llms.txt` indexes only published guides.

### Day frontmatter

```yaml
day: 1 # sort order
title: Hello World
board: waveshare-amoled-18-v2 # required key from src/lib/boards.json
summary: Put your first words on an AMOLED. Keep them there.
verification: Visual soak recorded # short label; explain scope in the guide
toolchain: ESP-IDF v5.5 # noted on every lesson — toolchains vary by day
firmware: /firmware/day-01-hello-world.bin # optional, hosted merged binary
video: https://youtu.be/VIDEO_ID # optional, embedded when present
```

## Boards

`src/lib/boards.json` defines each kit’s exact model, chip, product link, display,
and SVG illustration. Each lesson must select a registered `board` in frontmatter.
That identity drives its illustrated board card, install instructions, flasher
chip family, project-index label, and Markdown metadata.
The homepage derives published project counts from these assignments.
Adding a board to the workbench does not imply an existing binary supports it.

Illustrations are original stylized vectors in `public/images/boards/`, based on
manufacturer product references. They show the form of each kit, not a wiring diagram.

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
Only directories with a published `README.md` are collected.

Run `pnpm check:site` after building to verify guide/Markdown parity, heading
anchors, code excerpts, firmware manifests, and draft exclusion.
The deploy command runs this check before uploading.

## Writing a day

Keep the guide direct: result → requirements → run it → check the result →
how it works → build and change it → troubleshooting → takeaway.
Include exact working directories, artifact paths, and the scope of recorded
verification. Build success, serial output, and visual checks are distinct.
Use absolute web links so Markdown works outside the site.

Preserve the actual debugging experience in `STORY.md`. Avoid duplicating
current flashing instructions there; link to the guide. Days without a
published README stay outside the site and the agent index.

## Deploy

```sh
pnpm run deploy
```

Deploys `dist/` to Cloudflare Workers (static assets) with custom domains
`esptember.com` and `www.esptember.com` (configured in `wrangler.jsonc`;
the zone must be on the same Cloudflare account).
