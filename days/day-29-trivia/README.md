---
board: m5stack-stopwatch
day: 29
title: Trivia
toolchain: Arduino CLI + M5Unified / ESP-IDF v5.5 + Waveshare BSP (one half each)
firmware: /firmware/day-29-trivia.bin
summary: "The capstone: a two-board quiz show composing the month's packets, buttons, tones, and fairness rules."
verification: "Full 12-question game machine-verified across both boards: 10/12 with two deliberate wrongs"
---

## The result

A quiz show across two boards: the Waveshare 1.8 hosts — statement on screen, deck in flash, the truth in its pocket — and the StopWatch buzzes in: **A for TRUE, B for FALSE**.
Right answers chirp and buzz in your hand; wrong ones groan.
Scores track on both screens, and every answer packet carries the *press timestamp*, the fairness rule that keeps radio latency out of tie-breaking when more buzzers join.
This is the capstone day: ESP-NOW from day 20, button grammar and cues from day 10, and the cross-brand discipline the month kept sharpening.
This day has two firmware images, one per board.

## What you need

- **Boards:** the M5 StopWatch (buzzer) and the Waveshare AMOLED 1.8 (host).
- **Connection:** USB data cables for flashing. No network — the deck bakes into flash.
- **For the download:** [uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the `uvx` command below.
- **For source builds:** arduino-cli + M5Unified for the buzzer; ESP-IDF v5.5 for the host.

Flashing replaces the firmware currently on each board.

## Run it

Download both images:
[day-29-trivia.bin](https://esptember.com/firmware/day-29-trivia.bin) (StopWatch buzzer) and
[day-29-trivia-waveshare.bin](https://esptember.com/firmware/day-29-trivia-waveshare.bin) (AMOLED 1.8 host).

Flash each at `0x0`:

```sh
uvx esptool --chip esp32s3 --port STOPWATCH_PORT \
  write-flash 0x0 day-29-trivia.bin
uvx esptool --chip esp32s3 --port WAVESHARE_PORT \
  write-flash 0x0 day-29-trivia-waveshare.bin
```

Tap the host's screen to start; the statement appears on both boards; answer on the pushers.

## How it works

One packet type family runs the whole game — question, answer, verdict — and the answer's payload is the day's idea:

```c
  tr_msg_t msg = {TR_MAGIC, 1, currentQid, (uint8_t)(truthy ? 1 : 0), 0,
                  millis(), {0}};
```

That `millis()` is the **press timestamp**: the moment the thumb hit the pusher, on the player's own clock.
With one buzzer it's bookkeeping; with several it's justice — the host can rank *presses* instead of *packet arrivals*, so a buzzer with a slow radio path never loses a tie it won physically.
The plan called this latency fairness; the implementation is one field.

The host owns the deck and the truth.
Twelve original true/false statements bake into flash — several of them auditing this very series ("At 100 Hz tick rate, a 5 ms FreeRTOS delay rounds to zero": *true*, day 23 has the scars).
Answers are ruled on arrival, verdicts unicast back, and the buzzer's feedback comes from day 10's vocabulary: rising chirp + haptic for right, low groan for wrong.

Duplicate and stale answers are dropped by qid — mash the pushers all you like, the first answer per question is the answer.

## Check the result

- Host boots to a lobby; the buzzer says `waiting for host...`.
- Tap the host: the statement appears on both screens within a beat.
- Answering locks the buzzer (`locked in`), the host shows the ruling and the press time, and the buzzer chirps or groans with matching haptics.
- Twelve questions in, the host shows the final score and offers a new game.

**Recorded evidence · September 22, 2026:** A complete 12-question game was played under script control across both consoles — twelve questions broadcast, twelve answers ruled, twelve verdicts delivered with press timestamps, final score 10/12 exactly matching the two deliberately wrong answers injected by the script.

## Used resources

- Days 20 and 10: the ESP-NOW contract discipline and the feedback vocabulary, composed rather than re-learned.
- M5Stack's TriviaPOD — the commercial prior art on this same hardware family; closed content, so the deck here is original.
- [Open Trivia DB](https://opentdb.com/) — the keyless question source for the networked sequel.

## Build and change it

Clone the repository once:

```sh
git clone https://github.com/chantastic/esptember.git
cd esptember/days/day-29-trivia
```

Buzzer half: `./scripts/build-stopwatch.sh`, upload as in day 20.
Host half: `cd firmware/waveshare && idf.py build flash`.

The deck is a struct array — edit `DECK[]` and reflash.
The obvious extensions: more buzzers (the press timestamp is already there to rank them), and swapping the baked deck for live Open Trivia DB questions via day 21's provisioning.
