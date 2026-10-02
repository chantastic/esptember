# Agent Dial: the story

## Forking a product, not a picture

The OpenHarness dial is a round USB companion for coding agents.
Its repository holds firmware, a daemon, schematics, and a handful of design documents.

The documents describe Wi-Fi pairing, an on-screen STOP, a spoken answer, and a colored state ring around the rim.
Then you read the firmware, and none of that is there.
Voice is a tap on a mic button, stop is a physical key and a confirm, and the rim is just black.

So the fork follows the code.
Wherever this lesson says "upstream," it means what upstream's firmware does at commit `4689799`, not what its docs hope for.

## The byte that names the product

Upstream's frames start with `A5 48`.
The comment beside that `48` explains that a sibling product with the same board once got flashed by the wrong app, because nothing on the wire told them apart.

The Stopwatch is also an Espressif USB device, and upstream's host offers firmware to anything that greets it as `harness`.
An honest fork keeps the framing, the CRC, and the message names, and changes that one byte to `45`.
Upstream's host then never reads our frames, and never offers our board its firmware.

## What a harness will let you do

Nothing reaches into a harness from outside.
pi extensions and Claude Code hooks run inside the harness and call out.

pi's extension API can abort a turn and send a user message, so a pi tile gets Stop and Voice.
Claude Code's hooks can decide a permission request, and its documentation shows nothing that stops a turn, answers `AskUserQuestion`, or injects a prompt.
So a Claude Code tile gets approvals and nothing it can't keep.

The dial learned to hide what a session can't do rather than pretend.

## "The graphics are a little whack"

The first candidate drew everything with M5GFX primitives and bitmap fonts.
It passed every contract check, and it looked wrong on the desk.

Upstream renders with LVGL and Geist, and ships both as LVGL sources.
Its full font set would not fit the Stopwatch's app partition beside Wi-Fi, so the fork regenerates the same faces from upstream's own TTFs, with upstream's converter options, over a narrower range.
The screens now use upstream's styles, colors, and icons, one pixel right to center on the wider panel.

## A clock that ran backwards

The device replay then failed in a way the host model couldn't.
The loop sampled `now`, input handlers stamped `millis()` a moment later, and `now - stamp` wrapped to four billion.

The dial fell asleep instantly.
A recording stopped the moment it began.
Left alone, the same arithmetic would have declared a live hub silent.

Every stamped timer now measures a signed difference against `millis()`.

## What we learned

A fork is honest when it follows the code, credits it, and lists every place it leaves.

Harness neutrality comes from capabilities: each adapter claims only what its harness exposes, and the dial draws only what a session can do.

A contract can pass while the screen looks wrong, so the device's framebuffer is its own evidence.
So is the replay, which found a clock bug no host model could.
