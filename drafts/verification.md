# Media lesson verification

Hardware: connected Waveshare ESP32-S3-Touch-AMOLED-1.8 V2 on `/dev/cu.usbmodem2101`.
Esptool identified ESP32-S3 revision v0.2, 16 MB quad flash, and 8 MB embedded PSRAM.
Boot logs identified the CO5300 panel and CST816S-family touch controller.

Toolchain: ESP-IDF v5.5.5, LVGL 9.5.0, Waveshare BSP 2.0.3.

## Builds and host checks

All four projects passed `idf.py build merge-bin`.

| Project | Application bytes | App partition |
| --- | ---: | ---: |
| Day 03 | 626,080 | 3 MiB |
| Day 04 | 857,008 | 3 MiB |
| Day 05 | 522,528 | 3 MiB |
| Day 06 | 589,792 | 3 MiB |

- Still assets: 115,200 bytes each; decoded RGB565 values contain only black and the quantized ESPtember orange.
- GIF: FFprobe reports 160 × 160, 20 frames, 2.000 seconds.
- Raw movie: 184 × 224 × 2 bytes × 30 frames = 2,472,960 bytes.
- Movie merge: `flasher_args.json` includes the media file at `0x310000`; those bytes match the source movie inside `merged-binary.bin`.
- The documented raw FFmpeg conversion was exercised with a one-second synthetic MP4; it produced exactly ten frames (824,320 bytes).
- Regenerating all six media assets produced identical SHA-256 hashes.
- All four leading README code excerpts were compared with their actual `main.c` implementations after normalizing indentation.
- Astro builds successfully with three published pages; the draft lessons remain outside its collection.

## Board checks

These are serial/runtime checks, not visual certification of the panel.

- Day 03: booted and reached `READY`; observed stable internal heap (252,871 bytes) and PSRAM (8,237,084 bytes) through the first 50-second heartbeat.
- Day 04: booted and reached `READY`; observed the same stable memory readings through 110 seconds. No swipe events were received during the capture, so physical swipe navigation remains unverified.
- Day 05: GIF loaded successfully and completed 54 loops during the two-minute capture. Internal heap (294,479 bytes) and PSRAM (8,108,052 bytes) remained stable through the 110-second heartbeat. Loop callbacks were approximately 2.2 seconds apart, despite the source GIF's two-second duration.
- Day 06: SD initialization returned `ESP_ERR_TIMEOUT` (0x107); flash fallback reached `READY` with the expected 2,472,960 bytes and 30 frames. All eleven ten-second reports in the two-minute capture show 10.00 submitted fps and zero late deadlines. Internal heap settled from 335,287 to 335,067 bytes by the second report and stayed there; free PSRAM remained at 8,069,140 bytes. This proves fallback behavior for the observed mount failure; it does not prove SD playback or establish whether a card is absent or faulty.

## Still needed before publication

- Confirm the actual image colors, orientation, and continued panel operation.
- Confirm carousel swipes and stopping at both ends.
- Confirm the GIF's visual output and loop boundary.
- Confirm movie rendering and sustained playback; serial frame submissions alone do not establish panel frame rate.
- Test SD playback with a known card, plus missing/invalid media cases; record card size and filesystem.
- Complete an extended stability run before adding claims about long-term operation or publishing flashable lessons.

The writing skill requires hardware verification before publication, so no hosted download links have been added to the drafts.
