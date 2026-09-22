---
board: m5stack-stopwatch
day: 28
title: Note Taker
toolchain: Arduino CLI (esp32 core) + M5Unified + ArduinoJson
firmware: /firmware/day-28-note-taker.bin
summary: "A voice recorder that files its own paperwork: hold, speak, release — transcribed and posted to Memos."
verification: "Boot, PTT states, and provisioning verified; live transcribe+file pending keys"
---

## The result

A voice recorder that files its own paperwork.
Hold the crown and speak — up to thirty seconds into PSRAM — release, and the clip goes to Deepgram for transcription; the text posts itself to your [Memos](https://usememos.com) server.
The screen walks the pipeline by name: recording, transcribing, saving, **SAVED** with the note's text.
No keyboard ever existed and none was missed.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** M5Stack Stopwatch Dev Kit (ESP32-S3, C152): mic, PSRAM, battery — the pocketable one, which is the point.
- **Connection:** a USB data cable for one-time provisioning.
- **Services:** a free [Deepgram](https://console.deepgram.com) key, and a [Memos](https://usememos.com) instance (self-hosted; a `docker run` away) with an access token.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** [arduino-cli](https://arduino.github.io/arduino-cli/) with the esp32 core, M5Unified, and ArduinoJson.

The display is 466 × 466 pixels behind a circular aperture.
Flashing replaces the firmware currently on the board.

## Run it

Download [day-28-note-taker.bin](https://esptember.com/firmware/day-28-note-taker.bin) and flash it at `0x0`:

```sh
uvx esptool --chip esp32s3 --port PORT \
  write-flash 0x0 day-28-note-taker.bin
```

On first boot the board hosts the `esptember-setup` portal: join it from a phone and one form collects Wi-Fi, the Deepgram key, and the Memos URL + token (day 21's pattern, with per-day fields).
Serial remains the power-user path at 115200:

```
wifi YOUR_SSID YOUR_PASSWORD
dgkey YOUR_DEEPGRAM_KEY
memos https://memos.example.com YOUR_MEMOS_TOKEN
```

Everything persists in NVS; the Wi-Fi entry is day 22's shared namespace, so an already-provisioned board skips setup entirely.

## How it works

This is day 27's pipeline restructured around a moment instead of a stream.
Captions wanted words *during* speech; a memo wants them *after* — so instead of streaming, the crown records the whole clip into PSRAM (30 seconds of 8 kHz mono is 480 KB, pocket change against 8 MB), and release fires a one-shot pipeline:

Deepgram's **prerecorded** endpoint takes the raw buffer in a single POST — same honest audio-as-audio contract as the live socket, same `smart_format` punctuation.
The transcript then posts to Memos with one small JSON body and a bearer token:

```c
  http.begin(url + "/api/v1/memos");
  http.addHeader("Authorization", "Bearer " + token);
```

The pipeline runs blocking, by choice — a memo is a moment, and the moment can wait two seconds while the screen narrates `transcribing... → saving... → SAVED`.
Sub-half-second presses are discarded as pocket noise.
Failures name themselves on screen (`deepgram HTTP 401`, `no memos config`) because a device that files paperwork must also file its excuses.

## Check the result

- Idle: `hold A to speak`. Holding shows a red dot and a live seconds count; release shows the pipeline states in order.
- **SAVED** displays the transcribed text, word-wrapped, and the filed-notes counter increments.
- The note appears in your Memos timeline, punctuated and capitalized.
- Too-short presses return quietly to idle; a wrong key shows `FAILED` with the reason. B dismisses.

**Recorded evidence · September 22, 2026:** Boot, the PTT state machine, and the provisioning grammar were verified over serial (`D28_STATUS`). The live transcribe-and-file loop awaits a Deepgram key and a Memos instance on the bench, noted in NOTES.md.

## Used resources

- [Deepgram prerecorded API](https://developers.deepgram.com/docs/pre-recorded-audio) — raw linear16 in one POST.
- [Memos](https://usememos.com) `POST /api/v1/memos` with a bearer token.
- Days 22 and 27: the shared Wi-Fi namespace and the transcription contract.

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
cd days/day-28-note-taker
./scripts/build.sh
./scripts/flash.sh PORT
```

The merged image lands in `.build/firmware/note_taker.ino.merged.bin`.
Natural extensions: tag notes by holding B instead (`#idea` vs `#todo`), or queue failed uploads in NVS for retry — right now a dead network costs you the memo.
