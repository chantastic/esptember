---
board: waveshare-amoled-18-v2
day: 28
title: Real-Time Captions
toolchain: ESP-IDF v5.5 + Waveshare BSP (LVGL)
firmware: /firmware/day-28-captions.bin
summary: "The mic streams to Deepgram and words appear while you're still saying them — finals in white, hypotheses in gray."
verification: "Boot, portal, and pipeline states verified; live transcription pending a Deepgram key"
---

## The result

Speak near the board and words appear while you're still saying them.
The mic streams raw 16 kHz audio to [Deepgram](https://deepgram.com)'s live WebSocket; results come back in two flavors and the screen honors the difference — **finalized lines in white**, and the current **interim hypothesis in gray**, rewriting itself as context arrives.
Watching the gray line change its mind is the whole show.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2** (onboard mic).
- **Connection:** a USB data cable; a phone for the Wi-Fi portal.
- **An API key:** a free [Deepgram](https://console.deepgram.com) account — the signup credit covers months of captioning.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/) with its environment activated.

The display is 368 × 448 pixels.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-28-captions.bin](https://esptember.com/firmware/day-28-captions.bin) and flash it at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-28-captions.bin
```

Wi-Fi via day 22's portal, then the key over serial:

```
key YOUR_DEEPGRAM_KEY
```

The board reboots, connects, and starts listening.

## How it works

The whole pipeline is three reused parts and one new endpoint.
Day 15's mic capture reads 200 ms chunks; day 25's WebSocket discipline carries them; Deepgram's live endpoint takes raw PCM as binary messages, no framing, no base64:

```c
#define DG_URI                                                              \
    "wss://api.deepgram.com/v1/listen?encoding=linear16&sample_rate=16000&" \
    "channels=1&interim_results=true&smart_format=true"
```

The mic paces the stream, exactly as it paced day 24's walkie-talkie: each blocking read fills a chunk, each chunk sends whole.
No timers, no buffering strategy — the ADC is the metronome.

Results arrive as JSON with an `is_final` flag, and the two-tier display *is* the lesson about streaming STT: the service commits to words only after enough context arrives, so a live UI must render belief and fact differently.
Interim text repaints a gray line; finals push onto a scrolling white transcript:

```c
        if (cJSON_IsTrue(is_final)) {
            push_final(transcript->valuestring);
            interim[0] = 0;
```

The key follows the house rule — serial once, NVS forever, never in the repo or the binary.

## Check the result

- Without a key, the footer names the serial command; with one, it walks `connecting...` → `listening`.
- Speak: gray words appear within a beat, occasionally revising themselves mid-sentence.
- Pause: the gray line turns white and joins the transcript — `smart_format` adds punctuation and capitalization.
- Six finalized lines scroll; `D27_FINAL` logs each to serial.

**Recorded evidence · September 22, 2026:** Boot, the portal handoff, the key gate, and the capture→WebSocket→parse pipeline states were verified over serial. Live transcription awaits a Deepgram key on the bench, noted in NOTES.md.

## Used resources

- [Deepgram live streaming API](https://developers.deepgram.com/docs/getting-started-with-live-streaming-audio) — raw linear16 over WebSocket, `interim_results`, `smart_format`.
- Days 14, 21, and 24: the mic path, the portal, and the WebSocket client, composed.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember
```

With the ESP-IDF environment activated, run from the repository root:

```sh
cd days/day-28-captions/firmware
idf.py build
idf.py -p PORT flash monitor
```

To create the single downloadable image, run `idf.py merge-bin` in the same firmware directory.
Keep `pmu_init()` and `panel_reset_release()` before display startup when changing the UI.
The next lesson points this same pipeline at a different job: push-to-talk voice memos that file themselves.
