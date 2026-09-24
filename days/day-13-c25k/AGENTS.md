# C25K project guidance

Read [SPEC.md](SPEC.md) before changing this day's behavior, firmware, data format, or documentation.
It is the project's authoritative product plan.

- Update the relevant spec section alongside an agreed behavior or storage change, then align implementation, tests, and guides.
- If implementation differs from the plan, identify the mismatch and resolve it; do not silently rewrite the requirement to match a bug.
- Keep the workout table authoritative in SPEC.md and implemented in workouts.h. Derive on-device summaries from the program data.
- Keep operating/build procedures in BUILD.md, the published guide in README.md, and the build narrative in STORY.md. Link to the spec for the contract.
- Preserve the original spec in spec-history/v1.0.md. Add material changes to the current spec's revision history.
- Treat storage schema changes separately from document revisions. Define compatibility and migration before changing persisted data.
- Preserve existing device history during verification. Use RAM diagnostics or an isolated test namespace, and verify reset with an immediate reboot before any later write.
- Record unverified behavior and known limits in SPEC.md §14; keep observed verification results in BUILD.md.
- Run the checks relevant to the change as described in SPEC.md §12.1 and BUILD.md. Documentation-only changes do not require rebuilding or flashing firmware.
