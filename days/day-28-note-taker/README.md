---
board: m5stack-stopwatch
day: 28
title: Note Taker
toolchain: Arduino CLI (esp32 core) + M5Unified + ArduinoJson
firmware: /firmware/day-28-note-taker.bin
summary: "A voice recorder that writes: hold, speak, release — transcribed and filed locally, browsable on the pusher."
verification: "Boot, portal provisioning, and PTT states verified on Wi-Fi; live transcription pending a Deepgram key"
---

## The result

A voice recorder that writes.
Hold the crown and speak — up to thirty seconds into PSRAM — release, and the clip goes to Deepgram for transcription; the text files itself **locally**, into a ring of twenty notes that survive power loss and browse on the second pusher, newest first.
The screen walks the pipeline by name: recording, transcribing, saving, **SAVED** with the note's text.
No keyboard ever existed and none was missed — and no backend either: cloud filing is the extended-options section below.
Each day is a standalone firmware image; you can start here without flashing earlier days.

## What you need

- **Board:** M5Stack Stopwatch Dev Kit (ESP32-S3, C152): mic, PSRAM, battery — the pocketable one, which is the point.
- **Connection:** a USB data cable for one-time provisioning.
- **A service:** one free [Deepgram](https://console.deepgram.com) key for the speech-to-text. Nothing else — notes live on the device.
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

On first boot the board hosts the `esptember-setup` portal: join it from a phone and one form collects Wi-Fi and the Deepgram key (day 21's pattern, with per-day fields).
**Holding both pushers for two seconds** at any time forgets Wi-Fi and reopens the portal — reconfiguration never needs a computer.
Serial remains the power-user path at 115200:

```
wifi YOUR_SSID YOUR_PASSWORD
dgkey YOUR_DEEPGRAM_KEY
```

Everything persists in NVS; the Wi-Fi entry is day 22's shared namespace, so an already-provisioned board skips setup entirely.

## How it works

This is day 27's pipeline restructured around a moment instead of a stream.
Captions wanted words *during* speech; a memo wants them *after* — so instead of streaming, the crown records the whole clip into PSRAM (30 seconds of 8 kHz mono is 480 KB, pocket change against 8 MB), and release fires a one-shot pipeline:

Deepgram's **prerecorded** endpoint takes the raw buffer in a single POST — same honest audio-as-audio contract as the live socket, same `smart_format` punctuation.
The transcript then files into a local ring — twenty NVS slots and a cursor:

```c
static bool saveNoteLocal(const String &text) {
```

Local-first was a deliberate reversal: notes you dictate on a walk shouldn't depend on a server being reachable, and the B pusher browsing the ring makes the device complete by itself.

The pipeline runs blocking, by choice — a memo is a moment, and the moment can wait two seconds while the screen narrates `transcribing... → saving... → SAVED`.
Sub-half-second presses are discarded as pocket noise.
Failures name themselves on screen (`deepgram HTTP 401`, `no memos config`) because a device that files paperwork must also file its excuses.

## Check the result

- Idle: `hold A to speak`. Holding shows a red dot and a live seconds count; release shows the pipeline states in order.
- **SAVED** displays the transcribed text, word-wrapped, and the filed-notes counter increments.
- B cycles through saved notes, newest first; notes survive a power cycle.
- Too-short presses return quietly to idle; a wrong key shows `FAILED` with the reason. B dismisses.

**Recorded evidence · September 22, 2026:** Boot, the captive-portal provisioning (completed on the bench — the board holds Wi-Fi), and the PTT state machine were verified over serial (`D28_STATUS wifi=1`). Live transcription awaits a Deepgram key, noted in NOTES.md.

## Extended options: filing to a cloud

The local ring is the shipped core; every option below is one `saveNote` function away, and the migration prompt from day 26 applies with the obvious substitutions.

- **[Notion](https://developers.notion.com)** — the one most people already have: an internal integration token + one shared database; notes become rows. The strongest "real notes app" target.
- **[Telegram bot](https://core.telegram.org/bots/api#sendmessage)** — free forever, two strings (bot token, chat id), and every memo arrives on your phone with a notification, permanently searchable.
- **Discord webhook** — one URL, zero auth code, notes in a private channel.
- **[Memos](https://usememos.com)** — self-hosted timeline, `POST /api/v1/memos` with a bearer token; a Docker one-liner on any box you own.
- **Your own Worker** — Cloudflare Workers + D1 is SQLite at the edge; ~120 lines makes `memo.your.dev`, and keeping Memos' API shape means this firmware wouldn't know the difference.
- **Apple Notes** — has no API at all; the honest bridge is a queue (the Worker above) drained by a Mac running `osascript` into Notes.app, which iCloud then syncs everywhere. The notes app with no API gets one anyway.

## Used resources

- [Deepgram prerecorded API](https://developers.deepgram.com/docs/pre-recorded-audio) — raw linear16 in one POST.
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
Natural extensions: a cloud backend from the list above, tags by hold-duration, or export-over-serial for backing the ring up.
