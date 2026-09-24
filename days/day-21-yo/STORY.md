---
title: Yo
---
## The prompt-first pass

The original build below records how this idea first worked.
The durable lesson now targets the M5Stack Stopwatch and lives in the prompt, behavior contract, generalized tests, and hand-review checklist.
Generated firmware is evidence for those sources, not the source itself.


The plan for this day assumed three identical boards.
Reality supplied two different ones — an M5 StopWatch running Arduino and a Waveshare AMOLED running ESP-IDF — and that constraint improved the lesson, because it forces the honest question: what do two devices actually need to share to talk?

The answer is 24 bytes.

## The protocol is the only contract

Nothing else crosses the air.
The two halves share no code, no framework, no vendor, no build system — one is a `.ino` compiled by arduino-cli, the other a CMake project under ESP-IDF.
Their agreement is a packed struct and a channel number, and that was enough for mutual discovery in the first joint boot.

The `__attribute__((packed))` is load-bearing.
Two compilers padding a struct differently is the classic cross-platform radio bug — same source, different bytes — and packing is the one-word vaccine.

## Etiquette of a connectionless radio

ESP-NOW has exactly one social rule: anyone can *hear* a broadcast, but you must register a peer before you can *address* them.
So the roster protocol is: hear a hello, register the sender, and from then on unicast works.
Discovery costs nothing; conversation requires an introduction.
It's a cocktail party with MAC addresses.

The send callback is the other honest detail.
`esp_now_send` returning OK means the frame left; only the callback's `acked` means the other radio caught it.
Both consoles print the verdict per frame, because "sent" and "received" are different claims and day 13's verification culture applies to radios too.

## Native manners

The same poke lands differently on each board, deliberately.
The StopWatch buzzes and beeps — it has a motor and no touch.
The Waveshare floods its screen orange — it has a panel and no motor.
Heterogeneous hardware isn't an obstacle for the protocol layer; it's the *reason* the receipt layer exists.
Design the packet once, let each device answer in its own voice.

One trap crossed frameworks: the receive callback runs on the Wi-Fi task, and vibration is an I²C transaction that must not race the display's I²C from another task.
The callback sets a flag; the loop does the feeling.
Interrupt-context discipline is the same lesson on every RTOS — day 21's version just arrived by radio.

## The verification was the demo

Both consoles logged simultaneously while a script poked each direction: roster discovery, poke sent at 16.1 s, received at 16.1 s; reverse poke at 19.1 s, received at 19.1 s; every frame acked.
Two brands, two frameworks, one protocol, zero human hands.

## What we learned

- The wire format is the whole treaty. Two codebases that share one packed struct don't need to share anything else.
- `packed` is load-bearing. Compiler padding is the cross-platform radio bug you get exactly once.
- Broadcast is free; unicast needs an introduction. ESP-NOW's peer registry is etiquette, not bureaucracy.
- Let each device answer in its own voice. Protocol shared, manners native.
