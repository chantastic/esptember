---
board: m5stack-stopwatch
day: 23
title: Walkie-Talkie
toolchain: Arduino CLI + M5Unified / ESP-IDF v5.5 + Waveshare BSP (one half each)
firmware: /firmware/day-23-walkie-talkie.bin
summary: "Hold the crown and talk: mic audio streams over ESP-NOW, 22 logical channels, jitter-buffered into the speaker."
verification: "Test-tone stream machine-verified: 66/66 frames, 0 lost, 0 dropped"
---

## The result

Hold the StopWatch's crown and talk: mic audio streams over ESP-NOW — no router, no pairing — to the Waveshare 1.8, which plays it through its speaker.
Twenty-two logical channels ride one radio channel, exactly like FRS walkies share one slice of spectrum: the transmitter stamps each frame with a channel byte, the receiver squelch-drops everything not on its dial.
Day 20 proved the transport with 24-byte pokes; today saturates it with 16,000 bytes a second of voice.
This day has two firmware images, one per board.

## What you need

- **Boards:** the M5 StopWatch (transmitter: mic + PTT crown) and the Waveshare AMOLED 1.8 (receiver: speaker + channel touch buttons).
- **Connection:** USB data cables for flashing.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** arduino-cli + M5Unified for the TX half; ESP-IDF v5.5 for the RX half.

Flashing replaces the firmware currently on each board.

## Run it

Download both images:
[day-23-walkie-talkie.bin](https://esptember.com/firmware/day-23-walkie-talkie.bin) (StopWatch TX) and
[day-23-walkie-talkie-waveshare.bin](https://esptember.com/firmware/day-23-walkie-talkie-waveshare.bin) (AMOLED 1.8 RX).

Flash each at `0x0`:

```sh
uvx esptool --chip esp32s3 --port STOPWATCH_PORT \
  write-flash 0x0 day-23-walkie-talkie.bin
uvx esptool --chip esp32s3 --port WAVESHARE_PORT \
  write-flash 0x0 day-23-walkie-talkie-waveshare.bin
```

Match the channel numbers on both screens, hold the crown, and talk.

## How it works

The frame is the whole codec negotiation — there isn't one.
8 kHz, 16-bit, mono, 120 samples per frame: 15 ms of voice in 248 bytes, just under ESP-NOW's 250-byte ceiling:

```c
typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint16_t seq;
  uint8_t channel; // 1..22, logical
  uint8_t flags;   // bit 0: start of transmission
  int16_t pcm[FRAME_SAMPLES];
} wt_frame_t; // 8 + 240 = 248 bytes, under ESP-NOW's 250 limit
```

On the transmitter, the mic paces the radio: `record()` blocks ~15 ms filling a frame, which then broadcasts fire-and-forget.
Streams don't get per-frame retries — a lost 15 ms is a click, a *retried* 15 ms is lag for every frame behind it.

The receiver's jitter buffer is the whole art of audio over a lossy link: frames land in a queue from the radio callback, and playback doesn't start until four are waiting.
Those 60 ms of deliberate lag absorb the radio's unevenness so it never reaches the speaker.
Sequence numbers count losses; the codec runs at 16 kHz with each 8 kHz frame sample-doubled on the way out.

Half-duplex is free: it's push-to-talk, so echo cancellation — the hard problem of every speakerphone — simply never comes up.

## What went wrong

### The delay that wasn't

The receiver's audio task starved the idle watchdog on its very first boot, while doing *nothing*.
The idle loop slept with `vTaskDelay(pdMS_TO_TICKS(5))` — and at FreeRTOS's default 100 Hz tick, 5 ms rounds down to **zero ticks**, and a zero-tick delay never yields.
The polite little sleep was a busy loop wearing a disguise.
Any delay shorter than one tick is no delay at all; the fix is `vTaskDelay(1)` and knowing your tick rate.

## Check the result

- Both screens show channel 1. Hold the crown: **ON AIR** on the TX, **RECEIVING** on the RX, and your voice comes out of the Waveshare's speaker a beat later (the 60 ms prebuffer plus the radio).
- Change either channel so they differ: the RX goes quiet — squelch works. Match them again: audio returns.
- The TX's B pusher steps channels 1–22 with wraparound; the RX changes with on-screen +/−.

**Recorded evidence · September 22, 2026:** A deterministic 1-second 440 Hz test tone was streamed board-to-board under script control: 66 frames sent, 66 received, 0 lost to sequence gaps, 0 dropped at the queue, codec open confirmed (`D23_STATUS`, both consoles). Perceived audio quality through the speaker awaits an ear, noted in NOTES.md.

## Used resources

- ESP-NOW's 250-byte frame ceiling — the constraint that sized the whole audio format.
- Jitter-buffer fundamentals: prebuffer to absorb variance, conceal losses, never retry a stream.
- Day 20's transport lessons: packed structs, broadcast etiquette, callbacks on the Wi-Fi task.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember/days/day-23-walkie-talkie
```

TX half:

```sh
./scripts/build-stopwatch.sh
arduino-cli upload --fqbn 'esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi' \
  --port PORT --input-dir .build/firmware firmware/stopwatch/walkie_tx
```

RX half (ESP-IDF environment activated):

```sh
cd firmware/waveshare
idf.py build
idf.py -p PORT flash
```

The obvious v2 is symmetry — both boards have mics and speakers, so two-way is a matter of switching each side's I2S role on PTT.
The sequel lesson swaps the transport for the internet and keeps everything else.
