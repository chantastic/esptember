# Media lesson verification

Updated September 4, 2026 for the internet-media examples.
These results supersede the geometric-placeholder checks recorded in commit `37dcfc8`.

Hardware: Waveshare ESP32-S3-Touch-AMOLED-1.8 V2 on `/dev/cu.usbmodem2101`.
Earlier esptool identification confirmed ESP32-S3 revision v0.2, 16 MB quad flash, and 8 MB embedded PSRAM; the current boot logs identify CO5300 and CST816S-family controllers.
Toolchain: ESP-IDF v5.5.5, LVGL 9.5.0, Waveshare BSP 2.0.3.

## Builds and asset checks

All four revised projects passed `idf.py build merge-bin`.

| Project | Application bytes | App partition |
| --- | ---: | ---: |
| Day 03 | 626,080 | 3 MiB |
| Day 04 | 857,040 | 3 MiB |
| Day 05 | 1,100,432 | 3 MiB |
| Day 06 | 589,792 | 3 MiB |

- Portraits: four converted RGB565 files (one repeated for day 03), 240 × 240, 115,200 bytes each.
- Homer GIF: 368 × 448, 29 full-screen opaque frames; 583,146 encoded bytes. The selected RGB565 canvas requires 329,728 bytes before decoder overhead; the default ARGB8888 canvas would require 659,456. The frame validator rejects the previous asset (28 transparent frames, disposal 1) and accepts this one (zero transparent or partial frames).
- Big Buck Bunny: a saved three-second excerpt, converted to 30 raw frames at 368 × 224. The file is 4,945,920 bytes; two frame buffers use 329,728 bytes.
- Source attribution and hashes are in `assets/media/README.md` and `assets/media/SHA256SUMS`.
- Regeneration uses local source files. Asset reproducibility, dimensions, the movie's placement at `0x310000` in the merged binary, and README code excerpts passed after the final conversion.

## Board checks

These are serial/runtime checks, not visual certification of the panel.

- Day 03: the revised portrait firmware builds; its image bytes are identical to the gallery's first image. It has not been separately flashed in this revision.
- Day 04: the portrait gallery booted to `READY`. The 0-, 10-, and 20-second heartbeats reported unchanged internal heap (252,871 bytes) and PSRAM (8,237,084 bytes). No swipe events were received during this capture.
- Day 05: user reported that the previous asset's first frame looked correct but the bushes, wall, and white shirt turned black in later frames. Source inspection found LVGL 9.5 clearing alpha for transparent delta pixels instead of preserving the underlying image. The revision disables delta transparency and encodes full-screen, opaque frames; the full-screen ARGB8888 version completed six loops in a short capture, about 4.9 seconds apart. A follow-up RGB565 build reduces the decoded canvas and drawing work; it loaded successfully and produced loop callbacks about 3.6 seconds apart, with unchanged free internal heap (310,351 bytes) and PSRAM (7,876,628 bytes) in the observed heartbeats. The user confirmed that the fills and full-screen playback work correctly. The subsequent framing revision shifts the picture 26 pixels left (7% of screen width), keeping the same opaque-frame encoding.
- Day 06: scaling the initial 184 × 224 movie 2× on the board produced about 3.7 submitted fps and repeated late deadlines. The implementation now converts to 368 × 224 on the computer and draws at native size; the native-size version reports 10.01 fps with one initial late deadline, followed by 10.00 fps with zero late deadlines in the next two ten-second reports. Free PSRAM stayed at 7,901,204 bytes; internal heap settled at 326,107 bytes. These are short runtime observations, not a sustained playback benchmark.

SD initialization returned `ESP_ERR_TIMEOUT` during the first revised movie test; flash fallback worked.
That does not establish whether a card is absent or faulty, and SD playback remains unverified.

## Before publication

Confirm image colors and orientation, physical carousel swipes, GIF rendering and loop boundary, movie rendering, and sustained panel operation.
Test SD playback with a known card and record its capacity/filesystem, along with missing and invalid media cases.
Complete an extended stability run before claiming long-term stability.
Serial submissions are not a measurement of completed panel frames.

The writing skill requires hardware verification before publication, so the lesson READMEs remain drafts.
