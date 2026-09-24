# Day 09 testing model

Run the generalized contract validation:

```sh
scripts/prompt-contract/validate_day.sh days/day-09-sound-effects
```

The runner generates three ignored reducer candidates using list, indexed-table, and direct-function transition shapes.
It verifies defaults, field bounds, deterministic nonmutating reduction, immediate retrigger state, pusher paging, empty slots, 10,000 generated actions, Stopwatch geometry, shared touch-map metadata, localized controls, and all four round-face pad bounds.
It then removes the first transition from a fourth candidate and requires that mutation to fail.

This suite is intentionally portable.
It does not prove Arduino or LVGL integration, sensor or audio behavior, network services, persistence, framebuffer pixels, touch alignment, or physical controls.

For a generated candidate, domain-test every described recipe and converted file for deterministic PCM hash, duration, sample bounds, non-silence, and total PSRAM use.
Compile with pinned libraries, then use the `D09_` protocol to measure 100 injected pad triggers.
Pad-to-audio-start p95 must be at most 30 ms, no trigger may exceed 50 ms, and page state must change within 16 ms.

Capture both pages, complete 20 page round trips, verify the calibration record is unchanged, and finish [HAND-REVIEW.md](HAND-REVIEW.md) on the attached Stopwatch.
