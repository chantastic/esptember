---
title: Walkie-Talkie
---
## The prompt-first pass

The original build below records how this idea first worked.
The durable lesson now targets the M5Stack Stopwatch and lives in the prompt, behavior contract, generalized tests, and hand-review checklist.
Generated firmware is evidence for those sources, not the source itself.


Day 21 sent 24 bytes and called it a social network.
Day 24 sends sixteen thousand bytes a second and calls it a phone.
Same radio, same API — the difference is entirely in what you do about loss.

## Streams don't apologize

A poke that doesn't arrive should be retried.
A voice frame that doesn't arrive should be *forgotten* — by the time a retry lands, its moment has passed, and every frame behind it has aged 15 ms for nothing.
So the transmitter broadcasts fire-and-forget, and the receiver's sequence numbers exist to *count* losses, not to repair them.
A lost frame is a click; a retried frame is lag forever.
That inversion — reliability is harmful here — is the whole conceptual distance between day 21 and day 24.

## Sixty milliseconds of deliberate lag

The radio delivers frames unevenly; the speaker consumes them exactly on schedule.
Between those two clocks sits the jitter buffer: playback holds until four frames are queued, buying 60 ms of slack that absorbs the radio's stutter before it can reach the ear.
Voice tolerates lag beautifully and stutter not at all — the buffer trades the tolerable for the intolerable.

The mic paces everything else.
`record()` blocks until 15 ms of samples exist, so the transmit loop needs no timer: the ADC *is* the metronome.

## The delay that wasn't

The receiver's first boot starved the watchdog while idle, which made no sense — the idle path slept 5 ms per lap.
Except it didn't.
FreeRTOS ticks at 100 Hz by default; `pdMS_TO_TICKS(5)` is zero ticks; `vTaskDelay(0)` yields nothing.
The polite sleep was a busy loop in a disguise, and the watchdog saw through it.

The bug is humbling because the macro *looks* like it handles the conversion — it does, it just rounds down, and sub-tick delays round to lies.
Day 8's harness hit this watchdog from too much work; day 24 hit it from "no work at all."
Same alarm, opposite crimes.

## Squelch is one if statement

Twenty-two channels on one radio frequency is not radio engineering — it's a byte in the header and an early return in the receive callback.
FRS radios do the real version with sub-audible tones; the digital version is embarrassingly simple, and watching the RX go silent the moment the dials disagree is the day's most satisfying demo.

## What we learned

- Reliability can be harm. Streams want fire-and-forget; retries turn one lost frame into universal lag.
- Jitter buffers trade lag for smoothness. Sixty milliseconds nobody notices absorbs stutter everyone would.
- Sub-tick delays are zero. `pdMS_TO_TICKS(5)` at 100 Hz is 0, and `vTaskDelay(0)` never yields — know your tick.
- Channels are just data. Twenty-two walkie channels cost one header byte and one `if`.
