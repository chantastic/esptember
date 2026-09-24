---
title: Real-Time Captions
---
## The prompt-first pass

The original build below records how this idea first worked.
The durable lesson now targets the M5Stack Stopwatch and lives in the prompt, behavior contract, generalized tests, and hand-review checklist.
Generated firmware is evidence for those sources, not the source itself.


Somewhere in this workshop's history there's an earlier captions project, and this lesson was supposed to begin by excavating it.
The excavation never happened — and by the time the firmware was assembled it was clear why it didn't need to: after twenty-six days, live captions is not a project, it's an *afternoon of composition*.

Mic capture: day 15, unchanged.
WebSocket with auth headers over TLS: day 25, unchanged.
Provisioning: days 21 and the NVS key rule, unchanged.
The only new material is Deepgram's endpoint contract and one UI decision.

## The stream is the same stream

The revelation while wiring it: this is day 24's walkie-talkie with a smarter listener.
The mic paces the pipeline with blocking 200 ms reads; each chunk ships whole as a binary message; nothing buffers beyond the socket.
Where the walkie's far end was a speaker, this one is a speech model — but the transmitter literally could not tell the difference.

Deepgram meets that shape perfectly: raw PCM in, no framing, no base64, no envelope.
The URL carries the entire negotiation as query parameters.
APIs that accept audio the way audio already exists deserve their market share.

## Belief and fact want different pixels

Streaming STT has a two-phase honesty: the model emits *interims* — its current best guess — and only later commits *finals*, once context settles which "there/their" you meant.
A caption UI that renders both identically lies twice: it presents guesses as facts, then visibly "corrects" facts, which reads as failure.

So the screen has a caste system.
Gray text at the bottom: the hypothesis, allowed to rewrite itself shamelessly.
White lines above: committed transcript, append-only.
The moment of a gray line turning white — punctuation arriving with it, courtesy of `smart_format` — is the interface teaching the user how the model thinks.

It's day 20's confidence bar and day 15's "relative, uncalibrated" footer again: instruments that distinguish what they know from what they suspect.

## What we learned

- Late lessons are assemblies. Captions cost one new endpoint; the month had already built everything else.
- Send audio as audio. Deepgram's raw-PCM WebSocket meant zero encoding code on a microcontroller.
- Render belief and fact differently. Interim gray, final white — a streaming UI must show the model changing its mind without looking broken.
- The walkie-talkie generalizes. Mic-paced chunks over a socket serve speakers and speech models alike.
