# Day 08: LVGL Basics contract

Day 08 teaches the smallest useful LVGL application on the M5Stack Stopwatch.
It consumes the calibrated touch map produced by Day 07 and gives touch and the two physical buttons access to the same four controls.

## Portable candidate interface

Each disposable candidate supplies `lvgl_basics_core.h` and `lvgl_basics_core.cpp` with this API in namespace `lvglbasics`:

```cpp
enum class Screen : uint8_t { Controls, About };
enum class Focus : uint8_t { Count, Brightness, Orange, About };
enum class Input : uint8_t { None, Prev, Next, Enter, Escape };
enum class TouchTarget : uint8_t { Count, Brightness, Orange, About, Back };

struct State {
  Screen screen;
  Focus focus;
  bool editingBrightness;
  int pressCount;
  int brightness;
  bool orangeMode;
};

struct Rect { int x, y, width, height; };

inline constexpr int kDisplayWidth = 468;
inline constexpr int kDisplayHeight = 466;
inline constexpr int kPanelOffsetX = 6;
inline constexpr int kPanelOffsetY = 0;
inline constexpr int kMinimumBrightness = 10;
inline constexpr int kMaximumBrightness = 100;
inline constexpr int kBrightnessStep = 10;
inline constexpr int kFocusInset = 3;
inline constexpr size_t kControlCount = 4;

State initialState();
State reduce(State state, Input input);
State touch(State state, TouchTarget target, int value = 0);
bool valid(const State& state);
uint8_t panelBrightness(int percent);
Rect controlBounds(Focus control);
bool rectInsideRoundFace(Rect rect, int inset = 0);
```

## State behavior

`initialState` returns Controls, Count focus, brightness 100, count 0, Orange mode off, and brightness editing off.

On the Controls screen:

- Prev and Next wrap through Count, Brightness, Orange, About while not editing.
- Enter on Count increments once and leaves focus on Count.
- Enter on Brightness toggles editing.
- Prev and Next while editing change brightness by 10 and clamp inclusively to 10–100.
- Escape while editing exits editing and retains Brightness focus.
- Enter on Orange toggles Orange mode and retains focus.
- Enter on About opens the About screen, preserves all values, and retains About as the remembered focus.
- Escape while not editing focuses Count.

On the About screen, Enter and Escape return to Controls with About focused and every value preserved. Prev and Next do nothing.

Touch Count increments once and focuses Count. Touch Brightness clamps the supplied value to 10–100, updates it immediately, focuses Brightness, and leaves button-edit mode off. Touch Orange toggles it and focuses Orange. Touch About opens About and remembers About focus. Touch Back returns to Controls with About focus. A touch target that is not present on the current screen does nothing.

`valid` accepts only defined enum values, brightness within 10–100, and brightness editing while the Controls screen and Brightness focus are both active. Count must be nonnegative.

`panelBrightness` clamps the percentage to 10–100 and maps it to M5GFX's 0–255 range using `(percent * 255 + 50) / 100`.

## Layout contract

The interface fits the 468 × 466 drawable face without scrolling.

`controlBounds` returns the interactive bounds in this order:

| Control | x | y | width | height |
| --- | ---: | ---: | ---: | ---: |
| Count | 104 | 104 | 260 | 50 |
| Brightness | 104 | 200 | 260 | 42 |
| Orange | 272 | 266 | 92 | 50 |
| About | 104 | 330 | 260 | 50 |

All focus decoration is drawn inward by at least `kFocusInset` pixels.
`rectInsideRoundFace` checks all four inset rectangle corners against the circular face centered at `(234,233)` with usable radius 228 pixels and rejects non-positive rectangles or negative insets.

The Controls screen shows `LVGL basics`, a Count button and readout, a Brightness label and slider, an Orange mode label and switch, and an About button.
The About screen explains that a screen is a parentless widget and provides Back.
Keep both screens alive so Count, Brightness, Orange mode, focus, and edit state survive navigation.

Focus uses an inset border inside each widget's allocated bounds. The slider knob uses orange when focused and blue while editing. No focus decoration may extend outside its widget or be clipped by a parent.

## Stopwatch integration

Use Arduino ESP32 core 3.3.10, M5Unified 0.2.19, M5GFX 0.2.26, and LVGL 9.3.0.

After `M5.begin()`, set panel width 468, height 466, `offset_x = 6`, and `offset_y = 0`, then rotation 0. Create the LVGL display only after this correction. Draw nothing into rows 466 or 467.

Load `espt-touch/record` before creating the LVGL input device. Accept the Day 07 40-byte version-1 record and 440-byte version-2 record. For version 2, read raw controller points and apply the affine-plus-local warp; do not feed M5Unified's already converted coordinates to LVGL. Clamp only after applying the complete map.

If no valid map exists, enter the shared Day 07 guided calibration flow. A 600 ms two-button hold during boot always enters that flow. Committing a replacement map applies immediately and returns to this app; canceling retains the previous map.

App-only upload preserves the calibration namespace. A merged image flashed at address `0x0` is a fresh install and must be labeled as resetting calibration.

Use the C25K gesture grammar unchanged. With the lanyard down, BtnA is physical left and maps to Prev; BtnB is physical right and maps to Next; a brief chord is Enter; a 600 ms chord is Escape. Touch and buttons call the same state actions and LVGL events.

## Device evidence

Expose a serial observer and deterministic injected pointer/button devices that use the same state and LVGL event paths as physical input.

`status` reports board, display geometry, map version and generation, current screen, focus, edit state, count, brightness, Orange mode, touch availability, physical touch count, flush count, uptime, free heap, and free PSRAM.

Support `reset`, `tap X Y`, `drag X0 Y0 X1 Y1`, `button left`, `button right`, `button both`, `button hold`, `capture`, and `touchlog on|off`. Prefix Day 08 protocol lines with `D08_`.

Instrumented checks must cover every state transition, slider limits and an intermediate value, navigation persistence, input-chord isolation, screenshots of every focus state, and at least 20 About round trips without reset or growing memory use.

Physical acceptance confirms finger alignment using the active generation-2 map, button direction and chord feel, brightness perception, unclipped focus decoration, no bottom black bar, and successful entry into shared calibration.
