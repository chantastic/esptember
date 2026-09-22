---
title: Note Taker
---

The plan for this day once said "LVGL keyboard."
Sixty-some keys on a 1.75-inch circle, hunt-and-peck with a fingertip, to capture a thought before it evaporates.
The redesign happened the moment day 27 worked: if the device can hear, the keyboard is a skeuomorph.

Hold the crown.
Say the thought.
It files itself.

## A moment, not a stream

Captions and memos both turn speech to text, but they disagree about *when*.
Captions are a stream — words wanted mid-sentence, interims and finals, a socket held open forever.
A memo is a moment — nobody needs the transcript until the thought is finished.

That distinction picked the architecture: record the whole clip to PSRAM, then one POST to Deepgram's prerecorded endpoint.
No WebSocket, no interim states, no jitter thinking.
Thirty seconds of 8 kHz mono is 480 KB — the PSRAM that day 9 spent on a bubble sprite holds half a minute of your voice without noticing.

And the pipeline blocks, on purpose.
Two seconds of `transcribing... → saving...` after release is not lag to hide; it's the receipt printing.
The month's instruments kept learning to show their state by name — this one narrates its whole assembly line.

## The paperwork files its excuses too

A device that silently eats memos is worse than no device — you *trusted* it with the thought.
So every failure mode surfaces with its name: the HTTP code from Deepgram, the missing config, the sub-half-second press discarded as pocket noise.
The one weakness v1 accepts, and the README admits: a dead network at release time loses the memo.
The fix (queue clips in NVS, retry later) is designed but not built — day 12's rule about never promising rollback you don't have applies to promises about durability too.

## Three services, one rule, zero secrets

The provisioning grammar now carries three tenants — Wi-Fi (day 22's shared namespace, already set if the board did weather), a Deepgram key, a Memos URL + token — and the rule held without amendment: serial once, NVS forever, the published binary identical for everyone.
The last new lesson of the series' service arc is that there was no new lesson.
The pattern just kept accepting tenants.

## What we learned

- Keyboards are a fallback, not a default. On a device that hears, hold-and-speak beats hunt-and-peck by a category.
- Moments and streams are different architectures. Batch the memo; stream the caption; don't confuse the two.
- Blocking is honest when the user is waiting anyway. Narrate the pipeline instead of hiding it.
- Trust requires named failures. A note-taker's worst bug is a silent shrug.
