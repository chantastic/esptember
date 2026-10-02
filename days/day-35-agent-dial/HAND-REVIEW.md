# Agent Dial hand review

Reviewer: ____________________  Date: ____________________

Start with the hub running, the Stopwatch on USB, lanyard down, on the Overview.

## Fork fidelity

- [ ] Beside upstream's screenshots or firmware, the Overview, tile, recap card, working verb, question, drawer, voice overlay, `Cancel task?`, and toast read as the same product on a smaller face.
- [ ] Nothing on screen names OpenHarness features that aren't here (machines, tabs, engines, models).
- [ ] The completion beep sounds like upstream's triple tone and never plays twice within a second.

## Controls

- [ ] Left (BtnA) goes to the previous session and right (BtnB) to the next, with wrap; from the Overview either one opens the last session.
- [ ] Both briefly starts voice on a tile; both again sends. A both-hold of 600 ms never also fires Voice.
- [ ] Holding both on a question dismisses it; on a working pi tile it asks `Cancel task?`; on a working Claude Code tile it says `Can't stop this from here`.
- [ ] Swipes (55 px) page tiles, a bottom-edge swipe up toggles the Overview, and a top-edge swipe down opens the drawer.
- [ ] Taps land where the finger is across the whole face, including options near the rim and the chevrons.
- [ ] Settings › Touch calibration runs the shared flow; the new map applies without a reflash.

## With harnesses

Run one pi session and one Claude Code session in different directories.

- [ ] Both appear with the right name and harness label; the chevrons show with two sessions and hide with one.
- [ ] pi working: `Cooking… 12s` with the current command, redacted (`TOKEN=***`).
- [ ] pi finishing while you look at another tile: beep, a drawer row, and a badge on the Overview bell.
- [ ] Claude Code asking for a Bash permission: the dial wakes and beeps, shows Allow/Deny, and your tap decides the tool call in the terminal.
- [ ] Answering the same permission in the terminal first closes the question on the dial.
- [ ] Voice to pi: the bars move while you speak, the sparkles show while it routes, and pi starts a turn with your words.
- [ ] Voice with no `AGENT_DIAL_STT` configured: the toast says so.
- [ ] `Cancel task?` → `Yes, cancel` stops the pi turn.

## Connection and power

- [ ] Quit the hub: within about 15 s the dial shows `Not connected`; restarting any session reconnects it.
- [ ] Unplug USB with a paired board on Wi-Fi: adapter sessions keep updating directly.
- [ ] After 5 idle minutes the panel turns off; it stays on while a watched tile works or voice runs; the waking press does nothing else.
- [ ] Brightness changes are visible and persist across a reboot.
- [ ] No black bar at the bottom edge; nothing clipped at the rim.

## Findings

| Finding | Owner (lesson / Stopwatch skill / prompt-first skill) | Change |
| --- | --- | --- |
|  |  |  |
