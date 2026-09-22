---
board: waveshare-amoled-18-v2
day: 17
title: Pokedex
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL) + PokeAPI build-time fetch
firmware: /firmware/day-17-pokedex.bin
summary: "151 creatures, 2.7 MB of sprites, one fetch script. The asset pipeline grows up."
verification: "Boot with all 151 entries verified over serial; browsing pending hands-on"
---

## The result

The asset pipeline at scale.
Days 03–05 converted one image by hand; today a build-time script fetches 151 sprites and their species data from PokeAPI and generates 2.7 MB of C arrays — nothing committed, nothing hand-drawn, one command.
The UI is the list/detail pattern every data app uses: scroll the index, tap a row, meet the creature — sprite, types, size, and four stat bars.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2**: ESP32-S3, CO5300 panel, CST816-family touch, 16 MB flash, 8 MB PSRAM.
- **Connection:** a USB data cable and a computer with access to the serial port.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below. ESP-IDF is not needed.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/), plus Python with `requests` and `Pillow` for the asset fetch.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-17-pokedex.bin](https://esptember.com/firmware/day-17-pokedex.bin) and open a terminal in the download directory.
This is a merged image containing the bootloader, partition table, and application.

Find your serial port:

```sh
# macOS
ls /dev/cu.usbmodem*
# Linux
ls /dev/ttyACM*
```

On Windows, use the board's COM port from Device Manager.
Replace `PORT` with your port, then flash the image at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-17-pokedex.bin
```

Close any serial monitor using that port before flashing.
When flashing completes, the board restarts into this lesson.

## How it works

The pipeline is a script, not a ritual.
`scripts/fetch-assets.py` downloads each sprite and species record (cached locally), composites transparency onto black, converts to RGB565, and writes three generated files: the pixel arrays, the data table, and a header.
Generated sources are gitignored — the repo ships the pipeline, not the assets.

Storage is a taught budget, not an accident: 151 sprites × 96 × 96 × 2 bytes ≈ 2.7 MB, which outgrows the default 1 MB app partition.
The fix is a custom partition table — one CSV giving the factory app 6 MB of the 16 MB flash.

The detail screen is built once and *re-dressed* per entry.
An `lv_image_dsc_t` is just a header pointing at pixels; retarget it and the same widget shows any of the 151 sprites in flash:

```c
    sprite_dsc.data = (const uint8_t *)dex_sprites[index];
    lv_image_set_src(detail_sprite, &sprite_dsc);
```

No allocation while browsing, no copies — the sprite draws straight out of memory-mapped flash.

## What went wrong

### The 64 KB heap ambush

First flash: black screen, and the USB console so dead that esptool couldn't reach the chip.
The board was crash-looping — a `LoadProhibited` panic on every boot, rebooting faster than USB could enumerate.

The backtrace pointed at LVGL's theme code inside `lv_list_add_button`, on row after row of a 151-row list.
The real culprit was one missing config line: without `CONFIG_LV_USE_CLIB_MALLOC`, LVGL allocates from its builtin 64 KB pool instead of the system heap.
A hundred-some rows in, `lv_malloc` returned NULL and the theme applied itself to a null pointer.

One line of sdkconfig later the same firmware boots with 151 entries and room to spare — the system heap reaches PSRAM.

## Check the result

- The index lists `#001 Bulbasaur` through `#151 Mew` in finger-sized rows; the list scrolls with a flick.
- Tapping a row slides to the detail: sprite at double scale, name, types, height and weight, four stat bars.
- Back returns to the list at the same scroll position.

**Recorded evidence · September 22, 2026:** Boot with all 151 entries loaded was verified over serial after the heap fix (`D17_READY entries=151`), with the crash-loop diagnosis recorded above. Browsing and touch interaction await a hands-on check, noted in NOTES.md.

## Used resources

- [PokeAPI](https://pokeapi.co/) — free, keyless REST API and sprite repository. Sprite and name rights remain Nintendo/Game Freak's; assets are fetched at build time and never committed.
- ESP-IDF [custom partition tables](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/partition-tables.html).

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

Fetch the assets (once; they cache in `.build/asset-cache`):

```sh
cd days/day-17-pokedex
python3 scripts/fetch-assets.py
```

Then, with the ESP-IDF environment activated:

```sh
cd firmware
idf.py build
idf.py -p PORT flash monitor
```

To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
The output is `build/merged-binary.bin`.
Keep `pmu_init()` and `panel_reset_release()` before display startup when changing the UI.
The pipeline generalizes: point the fetch script at any image set and the same three generated files feed the same UI.
