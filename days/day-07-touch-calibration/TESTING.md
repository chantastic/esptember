# A reusable testing model for prompt-first lessons

A single test style cannot prove an embedded project.
The useful generalized suite has four layers, and every acceptance claim names the layer that supports it.

## 1. Portable contract

Put deterministic logic behind a small platform-neutral interface.

Test it on the host with strict compiler warnings and no Arduino dependencies.
Good candidates include transforms, parsers, state transitions, timing rules, validation, persistence encoding, and error thresholds.

Day 07's portable contract covers stable endpoint reduction, quick tap reduction, the deterministic Asteroid Shooter target schedule, affine fitting, two-direction perimeter coverage, equal direction weighting, local warp interpolation, smoothing, safety limits, and both saved-record formats.

## 2. Integration compile

Compile the hardware adapter with the real board libraries.

This catches API drift, unsupported language features, memory-layout assumptions, and accidental host-only dependencies.
A successful compile proves compatibility with the toolchain; it does not prove the device works.

## 3. Instrumented device contract

Give the firmware a small serial protocol for status, deterministic input injection where that is honest, and machine-readable results.

Use it to check state transitions, persistence across reboot, memory stability, and coordination between subsystems.
Injected input can prove routing and state behavior.
It cannot prove a physical sensor, button, speaker, motor, or display.

For runtime calibration, assert the full lifecycle: migrate a version-1 record without losing it, enter collection without reflashing, observe progress without perturbing it, cancel while retaining the old record, commit and apply a version-2 replacement immediately, reboot into the replacement, and preserve it across an app-only update. Test a fresh-install erase as a separate destructive installation path.

The instrumented path also verifies the fixed 51-target Asteroid Shooter schedule, its discarded tap-to-start input, target-only active frames, lower-ring coverage, deliberately injected misses, preserved residual vectors, one-step undo, and refusal to commit an incomplete field. These checks prove state and data routing. They do not prove where a physical finger touched.

Exercise the largest-memory transition, not only boot. Log a phase identifier, reset reason or boot marker, free memory, and task stack high-water mark before each screen. A host test can then distinguish an interaction problem from a reboot or stack exhaustion. Day 07 specifically requires its perimeter aggregate state to live outside the task stack; the first device candidate exposed this only when the four cardinal phases handed off to Around the World.

## 4. Physical acceptance

Keep a short list of observations that require the actual object.

For touch calibration, a person must complete the four center-to-direction slides, trace the visible ring once in each direction, tap every Asteroid Shooter target including some deliberate misses, inspect the before/after field, reboot the device, and confirm the new map remains active.
The result belongs in the lesson evidence with its date and hardware identity.

## Prompt discipline

When independent implementations disagree, inspect the prompt before fixing an implementation.

- If the behavior was unspecified, define it in the spec and add a contract test.
- If the behavior was already explicit, the candidate is wrong.
- If the behavior depends on hardware, add instrumentation and a physical acceptance step instead of pretending a host test proves it.
- Avoid source-text tests. Test the public interface, emitted protocol, or observable device behavior.

This keeps the prompt, specification, and acceptance criteria as the durable source.
Generated implementations can be replaced without weakening the lesson.
