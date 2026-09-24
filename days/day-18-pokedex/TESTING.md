# Day 18 testing model

Run the generalized contract validation:

```sh
scripts/prompt-contract/validate_day.sh days/day-18-pokedex
```

The runner generates three ignored reducer candidates using list, indexed-table, and direct-function transition shapes.
It verifies defaults, field bounds, deterministic nonmutating reduction, required scenarios, 10,000 generated actions, Stopwatch geometry, shared touch-map metadata, control direction, and round-face bounds.
It then removes the first transition from a fourth candidate and requires that mutation to fail.

This suite is intentionally portable.
It does not prove Arduino or LVGL integration, sensor or audio behavior, network services, persistence, framebuffer pixels, touch alignment, or physical controls.

For a generated device candidate, add domain tests for every numerical or protocol rule in SPEC.md, compile with pinned libraries, run the D18_ serial acceptance protocol, capture the declared reference state, and complete [HAND-REVIEW.md](HAND-REVIEW.md) on the attached Stopwatch.
