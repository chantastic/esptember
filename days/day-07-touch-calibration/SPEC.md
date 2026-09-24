# Touch Calibration contract

This contract separates the reusable calibration logic from M5Unified, M5GFX, Preferences, and the display UI.
An implementation that satisfies it can be exercised on a host computer before it is integrated with the Stopwatch.

## Candidate interface

Each candidate supplies two files:

```text
touch_calibration_core.h
touch_calibration_core.cpp
```

The header must expose this API in namespace `touchcal`:

```cpp
struct Point { double x, y; };
struct Affine { double a, b, c, d, e, f; };
struct Residual { double dx, dy; };
enum class TargetShape : uint8_t { FilledCircle, Ring, Hexagon, Asteroid };
struct AsteroidTarget {
  Point center;
  TargetShape shape;
  uint8_t visualRadius;
  uint32_t rgb;
  uint8_t radialLayer;
  uint8_t sector;
};
struct WarpMap {
  Affine affine;
  Residual center;
  Residual rings[3][16];
};
struct SampleResult { bool accepted; Point median; };
struct Verification { double meanError, maxError; bool passed; };
struct PerimeterCoverage {
  size_t sectorsCovered;
  size_t totalSamples;
  bool complete;
};

struct Record {
  uint32_t magic;
  uint16_t version;
  uint16_t width;
  uint16_t height;
  uint8_t rotation;
  uint8_t reserved;
  float coefficients[6];
  uint32_t checksum;
};

struct RecordV2 {
  uint32_t magic;
  uint16_t version;
  uint16_t width;
  uint16_t height;
  uint8_t rotation;
  uint8_t angularSectors;
  uint8_t radialRings;
  uint8_t reserved;
  uint32_t generation;
  float affine[6];
  float centerResidual[2];
  float residuals[3][16][2];
  uint32_t checksum;
};

inline constexpr int kDisplayWidth = 468;
inline constexpr int kDisplayHeight = 466;
inline constexpr int kRotation = 0;
inline constexpr size_t kCardinalCount = 4;
inline constexpr Point kCenter = {234, 233};
inline constexpr double kPerimeterRadius = 170.0;
inline constexpr size_t kPerimeterSectorCount = 16;
inline constexpr size_t kSamplesPerPerimeterSector = 4;
inline constexpr size_t kRadialRingCount = 3;
inline constexpr size_t kAsteroidAttemptCount = 51;
inline constexpr size_t kCenterAttemptCount = 3;
inline constexpr double kMaximumResidualMagnitude = 48.0;
inline constexpr double kMaximumNeighborDelta = 32.0;
inline constexpr size_t kMinimumSamples = 16;
inline constexpr uint32_t kMinimumSampleDurationMs = 80;
inline constexpr double kMaximumSampleSpan = 12.0;
inline constexpr double kMaximumMeanError = 12.0;
inline constexpr double kMaximumPointError = 22.0;
inline constexpr uint32_t kRecordMagic = 0x5443414c;
inline constexpr uint16_t kRecordVersion = 1;
inline constexpr uint16_t kRecordVersionV2 = 2;
inline constexpr char kNvsNamespace[] = "espt-touch";

extern const Point kCardinalTargets[kCardinalCount];
extern const double kRadialRings[kRadialRingCount];

SampleResult reduceSamples(const Point* samples, size_t count, uint32_t durationMs);
SampleResult reduceAttempt(const Point* samples, size_t count);
bool asteroidTarget(size_t attempt, AsteroidTarget* target);
bool fitAffine(const Point* raw, const Point* screen, size_t count, Affine* result);
Point apply(const Affine& transform, Point raw);
bool validAffine(const Affine& transform);
WarpMap zeroWarp(const Affine& transform);
Point applyWarp(const WarpMap& map, Point raw);
bool validWarp(const WarpMap& map);
void smoothRing(const Residual input[16], Residual output[16]);
void averageRings(const Residual clockwise[16], const Residual counterclockwise[16],
                  Residual output[16]);
Verification verify(const Point* expected, const Point* observed, size_t count);
size_t perimeterSector(Point point);
PerimeterCoverage analyzePerimeter(const Point* observed, size_t count);
uint32_t checksumRecord(const Record& record);
Record makeRecord(const Affine& transform);
bool validateRecord(const Record& record);
uint32_t checksumRecordV2(const RecordV2& record);
RecordV2 makeRecordV2(const WarpMap& map, uint32_t generation);
bool validateRecordV2(const RecordV2& record);
WarpMap warpFromV1(const Record& record);
```

`Record` must be 40 bytes.
The coefficient order is `a, b, c, d, e, f`, matching:

```text
screen_x = a*raw_x + b*raw_y + c
screen_y = d*raw_x + e*raw_y + f
```

`RecordV2` must be 440 bytes. It preserves the affine seed and adds a smooth local residual field for three radii and 16 angles plus one center residual. The fixed radial rings are 70, 120, and 170 pixels from display center.

## Required behavior

`kCardinalTargets` must contain `(234,63)`, `(234,403)`, `(64,233)`, and `(404,233)` in that order: up, down, left, right. Each is 170 pixels from `kCenter` at `(234,233)`.

The visible interaction is modeled after a controller-stick calibrator. Show a white center dot and one orange destination dot with no connecting line. For each cardinal step, use the exact sentence `Drag from the center {DIRECTION} to the orange dot`, replacing the token with UP, DOWN, LEFT, or RIGHT. The user touches the center, slides to the orange dot, and holds; a captured destination turns green. Firmware must not accept the stationary center as the endpoint. It begins the stable endpoint window only after raw movement of at least 80 units from the initial contact. Releasing early or returning toward center repeats that direction.

`reduceSamples` accepts only finite points with at least 16 samples collected over at least 80 ms.
It rejects a set when either axis has a max-minus-min span greater than 12.
For an even number of samples, the median is the mean of the two middle sorted values on each axis.

`reduceAttempt` accepts a touch release containing at least one finite point, ignores non-finite points when a finite point is also present, and returns the component-wise median of the finite points. It has no duration, span, or target-distance threshold because a quick miss is valid calibration data.

`asteroidTarget` returns false for a null output or an attempt outside `[0, 51)`. For a valid attempt, compute `id = (attempt * 20 + 7) % 51`. Ids 0–2 are center targets with `radialLayer = 0` and `sector = 255`; ids 3–18 use radius 70 with `radialLayer = 1` and sector `id - 3`; ids 19–34 use radius 120 with `radialLayer = 2` and sector `id - 19`; ids 35–50 use radius 170 with `radialLayer = 3` and sector `id - 35`. Sector 0 is up and sectors increase clockwise. Set shape from `attempt % 4`, visual radius from `{10,14,18,22}[(attempt * 3 + 1) % 4]`, and RGB color from `{0xff7a00,0x00d9ff,0x9dff00,0xff3bd4,0xffffff}[(attempt * 2 + 3) % 5]`. This deterministic permutation covers every required location exactly once while varying the target art independently of calibration truth.

`fitAffine` uses all supplied point pairs in an overdetermined least-squares fit.
It accepts four or more nondegenerate pairs, returns finite coefficients, and rejects singular input.
It must handle scale, offset, swapped axes, inverted axes, skew, and small measurement noise.

`validAffine` requires six finite coefficients and an absolute linear determinant `abs(a*e - b*d)` of at least `1e-9`.

`applyWarp` first applies the affine transform. It then interpolates a residual vector in polar display space: angular interpolation wraps between sector 15 and sector 0; radial interpolation runs from the center residual through rings at 70, 120, and 170 pixels; points outside 170 pixels use the outer-ring value. Add the interpolated residual to the affine point. This is a local scale-and-shape correction rather than a single pixel shift.

`zeroWarp` creates a map with the supplied affine transform and zero residuals, so its output exactly matches `apply`.

`validWarp` requires a valid affine transform, finite residuals, residual magnitude no greater than 48 pixels, and Euclidean difference no greater than 32 pixels between angular neighbors, radial neighbors, and the center and first ring. Angular neighbor validation wraps across sector 15/0. These bounds limit abrupt local warping caused by a noisy tap.

`smoothRing` applies a circular `0.25 previous + 0.5 current + 0.25 next` kernel independently to x and y. Sector 0 uses sector 15 as its previous neighbor. Build the stored inner, middle, and outer residual rings through this smoothing step so one imperfect tap cannot create a sharp local jump.

`averageRings` combines the clockwise and counterclockwise outer-ring measurements with equal weight in every sector, independent of how many raw samples either pass produced. It must permit either input array to alias the output. Keeping the two passes separate until this step exposes direction-dependent drag or controller hysteresis instead of allowing the longer pass to dominate the map.

## Guided data construction

The complete exercise has three ordered stages: four cardinal drags, two Around the World passes, and Asteroid Shooter. A new map is provisional until every required measurement exists and `validWarp` succeeds. Canceling at any stage discards the provisional data and leaves the active map and saved record unchanged.

The cardinal stage supplies the four raw/screen pairs for the affine seed. Its visible rule is strict: one white center dot, one orange destination dot, and no connecting line, ray, arrow shaft, or orange trail. The instruction is exactly `Drag from the center UP to the orange dot`, with DOWN, LEFT, and RIGHT substituted for the remaining stages.

Around the World collects two distinct 16-sector datasets in this order: clockwise, then counterclockwise. Each dataset requires at least four finite samples per sector, survives lifts, permits backtracking and extra rotations, and never rejects a sample for radial distance. For each sample, apply the affine seed, project the result to radius 170 without changing its observed angle, and accumulate `projected - affineMapped` as the residual. Reduce each pass to one residual per sector. Report the Euclidean difference between the two pass residuals per sector, combine them with `averageRings`, and pass the combined result through `smoothRing` to obtain the outer ring. Sample count cannot give one direction more weight than the other.

Asteroid Shooter begins with a separate black introduction screen containing only `ASTEROID SHOOTER` and `Tap to start`. The tap that starts the stage is discarded after release and never becomes calibration data.

The active test uses 51 target attempts in a deterministic order: three at the center, 16 at radius 70, 16 at radius 120, and 16 at radius 170. The three rings place targets on exact 22.5-degree sector nodes. The 170-pixel ring deliberately exercises the lower and outer display. Styles cycle across filled circles, outlined rings, hexagonal rocks, and irregular asteroids; visual radii cycle across 10, 14, 18, and 22 pixels; colors cycle across orange, cyan, lime, magenta, and white. Every style contains a 2-pixel white center pip. Style, size, and color never change the mathematical target center.

During those 51 attempts, draw a black background and the current target only. Do not draw a title, directions, attempt count, progress, footer, button hint, previous target, impact marker, miss vector, toast, or result. This rule leaves every display region available for measurement, especially the lower face. Advance directly to the next target after a touch release.

Every touch release containing at least one finite raw sample is an attempt, including a visible miss. Use a component-wise median when the contact produced more than one finite sample. Record target id, target center, raw median, affine-mapped point, residual `target - affineMapped`, and Euclidean miss distance. Never reject an attempt because the mapped touch is outside the visible target. BtnA/left may undo exactly the latest accepted attempt before collection proceeds.

Use the component-wise median of the three center residuals for `WarpMap::center`. Put each target residual into its matching angular node, then call `smoothRing` on all three completed rings. Combine the smoothed 170-pixel shooter ring with the smoothed, equal-direction-weight perimeter ring using `averageRings`; smooth that combined outer ring once more. Validate the whole map. If validation fails, identify and recollect only the measurements adjacent to the unsafe residual; do not restart completed regions.

`verify` measures Euclidean distance for each pair.
It passes when the mean error is at most 12 pixels and the maximum error is at most 22 pixels.
Zero points is a failure.

`perimeterSector` divides the round face into 16 equal angular sectors. Sector 0 is straight up and sector numbers increase clockwise. It returns `kPerimeterSectorCount` for a non-finite point.

`analyzePerimeter` is a collection-progress function, not an accuracy test. It counts finite observations in each sector and reports complete after every sector contains at least four samples. It accepts clockwise, counterclockwise, backtracking, multiple rotations, and lifting and resuming. It does not reject radial distance from the guide ring: those samples are the data used to calibrate the perimeter.

`makeRecord` fills the fixed metadata, casts the six coefficients to float in contract order, sets `reserved` to zero, and calculates the checksum.
`validateRecord` checks every metadata field, the exact 40-byte record size, finite coefficients, the determinant threshold, and checksum equality.

The checksum is CRC-32/ISO-HDLC: reflected polynomial `0xedb88320`, initial value `0xffffffff`, and final XOR `0xffffffff`.
Feed these fields in order, least-significant byte first:

1. `magic` as 4 bytes
2. `version` as 2 bytes
3. `width` as 2 bytes
4. `height` as 2 bytes
5. `rotation` as 1 byte
6. `reserved` as 1 byte
7. each coefficient's IEEE-754 float bit pattern as 4 bytes, in array order

Do not include `checksum` itself.

`RecordV2` uses the same CRC-32 algorithm. Feed every field before `checksum` in declaration order, least-significant byte first, including generation, the six affine floats, two center floats, and all residual floats in radial-major, sector-major, x/y order. `makeRecordV2` writes version 2 and the fixed geometry counts. `validateRecordV2` checks metadata, exact 440-byte size, checksum, generation-independent map validity, and all fixed dimensions. `warpFromV1` accepts only a valid version-1 record and returns its affine transform with zero residuals.

## Firmware integration acceptance

The host contract does not prove the physical device.
A conforming firmware must also satisfy these checks:

- Correct the panel to 468 × 466, offset `(6,0)`, before setting rotation 0.
- Read raw, unconverted coordinates while collecting calibration samples.
- Never draw into rows 466 or 467; no screen may leave a black strip at the bottom.
- Present the cardinal sequence as **UP, DOWN, LEFT, RIGHT**, with exactly one white center dot, one orange destination dot, no connecting line, and the required instruction sentence.
- Fit the transform from the four stable cardinal endpoints.
- Run **AROUND THE WORLD** clockwise and counterclockwise as two separately labeled collection passes. Render per-pass progress for all 16 sectors and finish each automatically when every sector has at least four samples. Never reject a sample because its radius differs from the guide ring.
- Record per-direction sector residuals and their difference. Combine the two direction rings with equal weight and smooth the result.
- Run **ASTEROID SHOOTER** with its separate tap-to-start screen and all 51 deterministic targets. During collection, render only the current target on black. Accept deliberate misses, preserve their residuals, and support undoing the latest attempt with BtnA/left.
- Construct and validate the center, 70 px, 120 px, and 170 px residual layers exactly as described above.
- Store bounded per-sector aggregate state outside large automatic task-stack arrays.
- Emit a machine-readable transition line and task stack high-water mark before each cardinal screen, each perimeter pass, and Asteroid Shooter. A complete physical run must reach every phase without a reboot or stack-canary failure.
- Save one 440-byte version-2 record under namespace `espt-touch` after the complete warp map validates. Increment its generation on every successful commit.
- Recognize a valid 40-byte version-1 record, load it as an affine transform with zero residuals, and offer a guided version-2 upgrade without discarding the usable map.
- Treat that record as device-owned runtime state; never compile device-specific coefficients into firmware.
- Read raw touch and apply the complete warp in the shared touch module for every application event. M5GFX's affine conversion alone is insufficient for version 2.
- Apply a newly committed map immediately and replace the record without requiring a build, flash, or reboot. Canceling calibration preserves the previously valid map and record.
- Reject invalid saved data and return to calibration.
- Every later Stopwatch application loads this record before creating its touch consumer and exposes the shared calibration flow through a 600 ms two-button hold during boot or an equally explicit settings action.
- Follow the C25K grammar: BtnA/left is Previous or Undo, BtnB/right is Next, a short two-button chord is Enter, and a 600 ms two-button hold is Back or Cancel. Chords must not leak single-button clicks.
- A brief two-button chord on the result screen runs the complete guided update. Holding both returns without deleting a valid map. The explicit `erase` serial command clears only the calibration record.
- Log raw samples, coefficients, per-direction sector diagnostics, target attempts, residual layers, load/save decisions, and final status over serial.
- Stream throttled live observation lines with phase, raw point, affine point, final mapped point, target or sector, residual, and progress. Observation must not affect collection behavior.
- After a reboot, load the saved map without silently recalibrating; allow the user to run a new guided update explicitly.
- Verify persistence with an app-only or OTA update. Treat a merged image written at address `0x0` as a fresh install that may erase NVS, and label it accordingly.
- Never claim physical accuracy from host tests, injected touches, or a successful compile.

## What the automated suite proves

The suite checks constants, cardinal placement, stable endpoint reduction, quick and imperfect tap reduction, the deterministic 51-target schedule including the outer ring, affine fitting, transform application, point verification, bidirectional perimeter collection, equal direction weighting, incomplete coverage, varied-radius traces, polar residual interpolation, asymmetric scale, circular smoothing, singular-input rejection, error thresholds, both record layouts, deterministic checksum behavior, tamper rejection, invalid maps, and version-1 migration.

The suite deliberately avoids source-text matching.
Candidates may use normal equations, QR decomposition, or another dependency-free method as long as their observable behavior matches the contract.
