---
title: Package Tracker
---

The plan flirted with doing this without an account — and the research verdict was clean: there is no keyless package tracking.
Every carrier gates its API behind registration; every aggregator wants a token; scraping tracking pages means fighting bot walls that exist precisely to stop you.

So day 26 stopped pretending and became the lesson it wanted to be all along: **your first keyed API**, handled the way every real integration handles it.

The provider went through its own drama.
The first build targeted 17TRACK; then a search-engine AI suggested two alternatives, one of which — a service with a too-good-to-be-true free tier — turned out to exist but with terms the AI had invented.
The other suggestion was the keeper: EasyPost's sandbox has *mock tracking numbers that simulate every delivery state*, which converts this lesson's weakest point — you can't demo a package tracker without a package in the mail — into its best feature.
Verify AI-recommended services against their own websites; keep the good idea, discard the embellishment.

## A key is just another secret

The month already built the machinery.
Day 21 established that secrets live in NVS and arrive through a provisioning channel; day 22 gave that channel a serial grammar.
The API key is one more `key ...` line and one more NVS field — the pattern absorbed its first paid-tier tenant without growing.

The design consequence is the one worth repeating: the binary on the website is identical for every downloader.
Nothing personal compiles in.
The *device* gets a key; the firmware never does.

## Register, then ask

EasyPost's API has a two-step shape that trips first-time users: a tracking number becomes a *tracker* object before queries about it answer.
It's an inversion of REST instinct — the API keeps state about your interests — and the firmware models it honestly with a `registered` flag per package and a catch-up pass before every fetch.
Re-registering is idempotent, so the flag can lie toward false safely.
When an API keeps state, your client keeps a belief about that state, and beliefs need to be cheap to repair.

## Truck time

The refresh runs every fifteen minutes, and the number is a position, not a limitation.
Packages move at the speed of trucks; polling faster manufactures load without manufacturing information.
The series has now made this citizenship argument three ways — weather at ten minutes, RFC 8628's `slow_down`, and now truck time — and it keeps being the same lesson: match your poll rate to the rate the world actually changes.

The `refresh` command exists because doorbells are real.
Politeness with an override beats politeness as a cage.

## What we learned

- There is no keyless package tracking. Sometimes the ecosystem's answer is "make an account," and the honest lesson is how to do that well.
- Sandboxes with mock data beat live data for lessons. Seven fake packages in seven states demo more than one real one in transit.
- Provisioning patterns compound. The key reused day 21's rule and day 22's grammar; the feature cost was one NVS field.
- Stateful APIs create client beliefs. Model the server's registry explicitly and make repairing the belief idempotent.
- Poll at the speed of the world. Trucks don't move in seconds; neither should your fetch timer.
