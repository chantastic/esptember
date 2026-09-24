# Day 13 testing model

Run the generalized contract validation:

```sh
scripts/prompt-contract/validate_day.sh days/day-13-c25k
```

The runner generates three ignored reducer candidates using list, indexed-table, and direct-function transition shapes.
It verifies defaults, field bounds, deterministic nonmutating reduction, required scenarios, 10,000 generated actions, Stopwatch geometry, shared touch-map metadata, control direction, and round-face bounds.
It then removes the first transition from a fourth candidate and requires that mutation to fail.

This suite is intentionally portable.
It does not prove Arduino or LVGL integration, sensor or audio behavior, network services, persistence, framebuffer pixels, touch alignment, or physical controls.

Day 13 also retains its stronger domain tests for the button recognizer, workout/session accounting, progress storage, and workout summaries.
Run those against any candidate directory that implements `buttons.h`, `session.h`, `progress.h`, `workout_summary.h`, and their documented dependencies:

```sh
days/day-13-c25k/scripts/test.sh PATH_TO_CANDIDATE
```

The previous generated implementation remains only in the ignored `.build/historical-source/` directory and is useful as one disposable candidate for this suite.

For a generated device candidate, add domain tests for every numerical or protocol rule in SPEC.md, compile with pinned libraries, run the D13_ serial acceptance protocol, capture the declared reference state, and complete [HAND-REVIEW.md](HAND-REVIEW.md) on the attached Stopwatch.
