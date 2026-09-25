# Pi Pet hand review

Reviewer: ____________________  Date: ____________________

Start in demo mode (no pi sessions running, or after `reset`), lanyard down.

## Character

- [ ] The pet reads as a Grok Bot avatar: a simple shape with expressive eye holes; each roster shape is recognizable.
- [ ] Edges are smooth at rest, mid-bounce, mid-spin, and while morphing between shapes or into glyphs.
- [ ] Blinks close the eyes to flat dashes and pop back open; expression changes blink through rather than sliding.
- [ ] Gaze moves feel deliberate; idle occasionally winks one eye.
- [ ] Working bobs and periodically spins, with the eyes travelling around the head and ribbons trailing.
- [ ] Thinking: the head contracts into the centre dot while the eyes turn away, and the outer dots spread out with a small overshoot; the highlight travels through size, lift, and brightness together.
- [ ] Blocked: the head drops into the lower dot while the tapered stem drops in, then shakes briefly every couple of seconds; Thinking ↔ Blocked blends directly.
- [ ] Settled eyes show their curved contours (bean and tilted shapes), not straight capsules.
- [ ] The black background shows no smear or trailing behind moving edges on this panel.
- [ ] Done spins and celebrates with ribbon lanes, then settles; waiting sags, half-lidded, with an occasional sigh.
- [ ] Motion is smooth, with no visible stutter; eyes never poke outside the head or merge.

## Demo controls

- [ ] BtnA/left goes to the previous bot and BtnB/right to the next; the head shape and color morph smoothly.
- [ ] Briefly pressing both cycles states; idle, happy, curious, excited, surprised, thinking, working, waiting, blocked, done, and sad are each distinct.
- [ ] Holding both for 600 ms sleeps: closed-line eyes, drifting `z`, dim screen. A page press wakes it.
- [ ] Tapping the pet makes it bounce (sometimes spin); dragging moves it and releasing wobbles it home; tapping elsewhere makes it look there.
- [ ] Touch lands where the finger is across the whole face, including near the edges.
- [ ] Tilting slides the pet downhill and it looks downhill; a firm shake makes it hop. Note the actual direction: ________

## With pi

Start two or three pi sessions in different directories.

- [ ] Each session appears with its own color; the name and activity line are readable and unclipped.
- [ ] The page dots match the sessions; the focused dot is larger; rings read at a glance: amber working, white thinking, red error, pulsing blue waiting for you.
- [ ] A red style under a red error ring is still distinguishable: ______
- [ ] Sounds: a new session pops, a run start ticks once (no chatter during tool bursts), done chimes, an error winces, and a persisting error says uh-oh; the level suits a desk; holding both mutes.
- [ ] While an agent works, its pet shows thinking dots (after a moment) and works while a tool runs; the activity line shows the command or file.
- [ ] A failing command makes the pet look surprised; a session that stops on an error shows `!` until acknowledged.
- [ ] When an agent finishes, its pet celebrates, then waits with a blue badge until acknowledged; an unfocused finished pet's dot pulses blue.
- [ ] Left/right page between sessions; a page stops that pet asking. Both-briefly acknowledges.
- [ ] Quitting a pi session removes its pet; quitting the last one returns to the demo bot.
- [ ] A command containing `TOKEN=...` shows `***` on the device.
- [ ] Bright backgrounds (DevBot) show no black bar at the bottom.

## Findings

| Finding | Owner (lesson / Stopwatch skill / prompt-first skill) | Change |
| --- | --- | --- |
|  |  |  |
