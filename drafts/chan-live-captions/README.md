# chan.dev live captions reference client

Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2** client for
`wss://devices.chan.dev/v1/transcriptions/deepgram/live`. This is a separate,
unpublished client project, not a replacement for the day-27/day-32 guides.
The reusable service belongs to `chan-services`; only hardware/audio/UI and the
public-client session live here.

## Use

The attached Waveshare was updated on September 23, 2026 with user approval.
Wi-Fi and `authkit` NVS entries were preserved; no provider key was installed.
Tap **START** to stream microphone audio and show interim/final captions; tap
**STOP** to drain final words and close the stream. A normal Start has no clip
limit. The service is billable. Failed connections do not automatically retry.

The existing Production Devices client ID is required. Access/refresh tokens are
still credentials and remain private on the device. No WorkOS environment key
or Deepgram API key is embedded, requested over serial, or logged. The device
uses the existing WorkOS device grant/refresh and Auth workspace discovery.

The display keeps one flowing transcript, not a permanent recording. Committed
words are light; the provisional tail is slightly dimmer in the same paragraph.
Finalization can revise/brighten the tail in place, without moving it into another
section or inserting a newline. Scrolling back freezes the displayed history
while incoming events continue into the bounded model. Return to the bottom to
resume following live text. “Interrupted” means
coverage is incomplete, not a successful stop. Long-session token renewal is
implemented, but a multi-hour/through-expiry hardware soak is not yet recorded.

## Build / narrowly scoped install

Use the existing ESP-IDF v5.5 toolchain and locked dependencies:

```sh
cd drafts/chan-live-captions/firmware
idf.py build
```

The working day-32 partition table matches this project exactly: NVS at `0x9000`
(size `0x6000`), application at `0x10000` (4 MiB). The original day-32 application
was verified against the attached device before replacement. For this verified
layout, install **only the application**:

```sh
python -m esptool --chip esp32s3 --port VERIFIED_PORT --baud 460800 \
  write_flash 0x10000 build/chan-live-captions.bin
```

Do not flash a merged image at zero, erase flash, read/export NVS, replace
bootloader/partitions, or assume another ESP board has this layout. Rediscover
and verify the port/board before every hardware session. The old day-32 app
binary/source remain available for rollback at the same application offset.
Firmware binaries, serial output, captures, and local build state stay out of Git.

## Diagnostics

One USB serial connection at 115200; commands are plain lines:

- `s`: safe state, event/frame counts, memory and display-update counters.
- `start`, `stop`: explicit stream control; start can incur provider charges.
- `test N`: explicit bounded stream, 1–120 seconds; stops and drains results.
- `preview`: local inline sample text with microphone **off**; no provider operation.
- `preview-final`: promote that sample tail in place (local only).
- `preview-history`, `preview-up`, `preview-bottom`: local long-history/follow-mode checks.
- `capture`: when stopped, `LIVE_CAPTURE 368 448 RGB565LE`, 448 base64 rows
  (736 decoded bytes each), then `LIVE_CAPTURE_END`. Captures can contain private
  text. Keep them outside Git and never treat a framebuffer snapshot as proof
  of physical panel signal quality.

No commands export session credentials, audio, or raw provider responses.
Only text lengths/counts are printed for recognition events, not the words.

## Hardware corrections found during verification

1. `CONFIG_WS_BUFFER_SIZE=18432` is necessary for AuthKit bearer headers. The
   socket's runtime `.buffer_size` does **not** enlarge ESP-IDF's separate 1 KiB
   HTTP-upgrade buffer. Low-level transport logging is disabled because its
   error path can print Authorization headers.
2. The BSP default display path used a non-DMA draw buffer/RGB helper for this
   QSPI panel. Under simultaneous TLS/audio it repeatedly failed to allocate SPI
   transmit buffers. `display.h` uses the normal SPI LVGL port and a preallocated
   internal DMA buffer of 368 × 16 pixels, preserving the board's panel/touch
   initialization and two-pixel rounding. No managed vendor files are patched.
3. Labels update only on changed content. LVGL LONG_DOT mutates its internal
   text, so the interim label compares an independent original-text cache.
   Comparing against `lv_label_get_text()` caused constant redraw even when idle.
4. An in-flight microphone chunk finishes before CloseStream. The socket is not
   destroyed while the capture task is using it.

## Verification — September 23, 2026

- Actual device → `devices.chan.dev` → private Auth → personal Deepgram Pipe:
  369 × 6,400-byte frames (**73.8 s**) with 57 interim and 32 final events,
  including empty final events, followed by `LIVE_STOPPED clean=1`.
- The user reported display corruption despite the initial network success.
  Hardware diagnostics established SPI DMA allocation failures; that earlier
  network result is not claimed as visual success.
- After the display fix, an 8.2-second test delivered 6 interim/4 final events
  and a clean stop. One preceding short run interrupted; the UI reported the
  interruption rather than pretending all audio was transcribed.
- Reviewed actual-device framebuffer captures. Two local-preview captures 12 s
  apart were byte-identical. UI update count held at 22, DMA free memory ~100 KiB,
  largest free DMA block 31,744 bytes. The screenshot shows readable finals,
  separate gray interim text and Start. Physical-panel confirmation remains a
  separate user observation; no camera/photo was captured.
- Final flashed app SHA-256:
  `f7f4ff42f51cb7a2ea9d9c73251edafa2559a25de62e20a739c7e8fbbbceaf76`.

Service protocol, tests and release versions are maintained in the canonical
`chan-services/docs/devices-live-transcription.md` and `docs/deployment.md`.
No website, other board, WorkOS provider configuration, or social post was changed.

### Later UI follow-up — September 23, 2026

The user confirmed the repaired screen was readable but disliked the split
interim/final layout and poor scroll-back. The installed UI now uses two colored
spans within **one paragraph**, with identical font/spacing and no newline at
segment finalization. It uses revision-based redraws. Touch scrolling switches
out of follow-live mode: pending recognition remains in the model without
re-laying out the text being read. Returning to the bottom applies pending text.
The preallocated DMA strip is now 368 × 32 pixels (rather than the initial 16),
reducing the number of draw passes per scroll without reintroducing SPI copies.

The host C test passed under address/undefined-behavior sanitizers: inline
promotion, replacing interim text, empty final events, no-op revisions, bounded
history and UTF-8-safe word-boundary trimming. Firmware built and only the app
partition was flashed; service versions and login/Wi-Fi storage were unchanged.
On-device local captures show the same words at the same positions before and
after promotion, with only the provisional color changing. A history-view
capture was byte-identical before/after tail finalization; returning to the
bottom showed the pending final text. No new speech/provider test was needed
for this UI change. Physical scroll feel still requires user confirmation.

Current flashed app SHA-256:
`0e7d897222b3660a474dc8706ff8fcc935d973f6c3ac46678a15e0c48e2da375`.
The device was left stopped in the local inline preview; Start clears it.

