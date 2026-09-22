---
title: A Movie
---

Day 05 played motion something else had encoded.
Day 06 builds the player itself — raw frames, our loop, our budgets — because a movie is the first asset in this series that is *bigger than everything*: bigger than RAM, bigger than the app partition, eventually bigger than the flash chip.

That's the assignment inside the assignment: make peace with data that never fits.

## Three budgets, no overlaps

A movie spends from three separate accounts.
Storage: 4.9 MB of flash for three seconds.
Memory: two 165 KB frame buffers — the whole clip never needs to be resident, because frames arrive one at a time.
Time: 100 ms to read, copy, and invalidate each frame, or the rate slips.

The budgets don't trade against each other, and the day's two failures were each an overdraft in exactly one account.

## Resize where the CPU is cheap

The first build stored half-width frames and scaled them 2× on the board — a storage optimization paid for out of the time budget.
3.7 fps, endless late deadlines.
The desktop that converted the movie could have produced full-width frames for free, so it now does: twice the flash, zero board-side scaling, and the rate snapped to 10.00 even.

The general rule outlived the day: do work where the work is cheap.
Day 17's build-time sprite pipeline is this decision again at scale.

## The partition table is the API

The default flash layout gives an app 1 MiB and calls it a day.
This movie needed a 12 MiB raw region with no filesystem, and getting it was five lines of CSV — the same lesson days 17, 20, and 24 later drew on.
Flash is not a fixed shape; it's a budget you file paperwork for.

## The fallback that almost lied

The player falls back to the flash clip when SD mounting fails — good behavior, dangerous evidence.
The first SD test timed out, the fallback rolled, the bunny played, and for a moment the screen said *success*.
Only the `source=flash` line in the log told the truth: the thing being tested had not been tested.

So the claim stayed honest — SD playback is recorded as **unverified** — and the log line that distinguishes the paths is now mandatory equipment.
Day 05 learned that logs can be too happy; day 06 learned the complement: a *screen* can be too happy, and sometimes the log is the honest one.
You need both, disagreeing rarely.

## What we learned

- Big data is a streaming problem. The movie never fits in RAM and never needs to — two buffers and a read loop beat any amount of memory.
- Resize where CPU is cheap. Desktop FFmpeg work bought back 6.3 fps of board time at the price of flash nobody missed.
- Partition tables are design surface, not configuration. Twelve raw MiB was five lines of CSV — the series kept cashing this check.
- Fallbacks hide the untested path. `source=flash` versus `source=SD` is one log line, and it's the difference between a passed test and a plausible lie.
