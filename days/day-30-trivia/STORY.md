---
title: Trivia
---

This lesson was originally slotted eleven days earlier, right after the first ESP-NOW day, and it kept getting pushed back — correctly, it turns out.
Trivia looked simple and was actually five lessons in a trenchcoat: packet design, game state sync, input grammar, feedback cues, and a fairness problem.
By day 30, four of the five were already on the shelf.

The capstone wrote itself out of parts.

## Fairness is a field, not a feature

The one genuinely new idea is in the answer packet: the player's own `millis()` at the moment of the press.

With a single buzzer it's trivia trivia — a number on the host's screen.
But the whole design of a buzz-in game collapses if arrival time decides ties: radio retries, channel contention, one board's Wi-Fi task having a moment — all of it becomes gameplay.
Stamping the press at the source moves the race from the network to the thumbs, where it belongs.
Multi-buzzer ranking becomes a sort on data the protocol already carries.

Fairness cost four bytes.
Most fairness does, if you design it in before you need it.

## The deck audits the month

Writing original true/false statements beat licensing someone's question bank, and the deck became a self-referential exam: *Bluetooth Classic works on the ESP32-S3* (false — the iPod died for this), *a 5 ms FreeRTOS delay rounds to zero at 100 Hz* (true — day 24's watchdog has the scars), *AMOLED screens need a backlight* (false — day 1 learned it the hard way).
A trivia deck that teaches the course it ships with.

## The game that played itself

The verification run is the series' thesis in one transcript: a script advanced the host's deck over one serial port, answered on the buzzer's pushers-by-proxy over the other, and injected exactly two wrong answers.
Final score: 10 of 12.
Twelve questions, twelve rulings, twelve verdicts, two brands, two frameworks, zero humans — and the two errors were the two the script chose.

When the *mistakes* are machine-verified, the game logic is done.

## What we learned

- Capstones compose; they don't introduce. Four of five subsystems came off the shelf, and it showed in the build time.
- Fairness is cheapest at design time. One timestamp field future-proofs the entire multi-buzzer problem.
- Write your own deck. Original questions dodge licensing and — better — get to teach the course they belong to.
- Verify the errors. A test that only proves success proves half; injecting known wrongs proves the ruling.
