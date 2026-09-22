// ESPtember Day 09 — Level
// The round face becomes a bubble level, in the spirit of the Apple
// Watch Ultra's: a bubble that drifts with tilt, live degree readouts,
// and a snap-to-green moment — with a haptic tick — when the device
// lies flat within a degree. The BMI270 supplies acceleration; the
// stationary reading is the support force pointing opposite gravity,
// not gravity itself (do not reverse the mapping).
#include <M5Unified.h>
#include <math.h>

// --- Tilt from acceleration ------------------------------------------------
// Face-up on a table, the support force is +Z. Tilt shifts components
// into X and Y; two atan2s recover pitch and roll in degrees. A low-pass
// filter (alpha 0.15 at 50 Hz) steadies the bubble without lag you can
// feel.
static float pitchDeg = 0, rollDeg = 0;

static bool readTilt() {
  if (!(M5.Imu.update() & m5::IMU_Class::sensor_mask_accel)) return false;
  auto data = M5.Imu.getImuData();
  const float ax = data.accel.x, ay = data.accel.y, az = data.accel.z;
  const float p = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / (float)M_PI;
  const float r = atan2f(ay, az) * 180.0f / (float)M_PI;
  const float alpha = 0.15f;
  pitchDeg += alpha * (p - pitchDeg);
  rollDeg += alpha * (r - rollDeg);
  return true;
}

// --- Level state -------------------------------------------------------------
#define LEVEL_TOLERANCE_DEG 1.0f
#define RANGE_DEG 30.0f // bubble hits the ring at this tilt

static bool isLevel = false;
static uint32_t vibeOffAt = 0;

static void updateVibe() {
  if (vibeOffAt && int32_t(millis() - vibeOffAt) >= 0) {
    M5.Power.setVibration(0);
    vibeOffAt = 0;
  }
}

// --- Rendering ---------------------------------------------------------------
// A full-screen sprite in PSRAM: the bubble animates at 30 fps with no
// flicker because every frame is composed off-screen and pushed whole.
static M5Canvas canvas(&M5.Display);

#define CX 233
#define CY 233
#define RING_R 190
#define BUBBLE_R 34

static void render() {
  const uint16_t accent = isLevel ? 0x07E0 /* green */ : 0xFD20 /* orange */;
  canvas.fillSprite(TFT_BLACK);

  // Reference rings and crosshair.
  canvas.drawCircle(CX, CY, RING_R, accent);
  canvas.drawCircle(CX, CY, BUBBLE_R + 4, accent);
  canvas.drawFastHLine(CX - RING_R, CY, 2 * RING_R, 0x39E7);
  canvas.drawFastVLine(CX, CY - RING_R, 2 * RING_R, 0x39E7);

  // The bubble floats opposite the tilt, like the air bubble in a vial:
  // tip the right side down and the bubble escapes left.
  float bx = -rollDeg / RANGE_DEG, by = -pitchDeg / RANGE_DEG;
  const float mag = sqrtf(bx * bx + by * by);
  if (mag > 1.0f) { bx /= mag; by /= mag; } // pin to the ring
  const int px = CX + (int)(bx * (RING_R - BUBBLE_R - 6));
  const int py = CY + (int)(by * (RING_R - BUBBLE_R - 6));
  if (isLevel) canvas.fillCircle(CX, CY, BUBBLE_R, 0x07E0);
  else {
    canvas.fillCircle(px, py, BUBBLE_R, TFT_WHITE);
    canvas.drawCircle(px, py, BUBBLE_R, accent);
  }

  // Degree readouts.
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(accent, TFT_BLACK);
  canvas.setTextSize(3);
  char line[24];
  if (isLevel) {
    canvas.drawString("LEVEL", CX, CY - 90);
  } else {
    snprintf(line, sizeof(line), "%+.1f", (double)rollDeg);
    canvas.drawString(line, CX, 60);
    snprintf(line, sizeof(line), "%+.1f", (double)pitchDeg);
    canvas.drawString(line, 60, CY);
  }
  canvas.pushSprite(0, 0);
}

static void reportStatus() {
  Serial.printf("D09_STATUS pitch=%.2f roll=%.2f level=%d\n", (double)pitchDeg,
                (double)rollDeg, (int)isLevel);
}

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  cfg.internal_imu = true;
  cfg.internal_rtc = true;
  cfg.internal_mic = false;
  cfg.internal_spk = true;
  cfg.fallback_board = m5::board_t::board_M5StopWatch;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(255);
  M5.Power.setVibration(0);
  canvas.setColorDepth(16);
  canvas.createSprite(466, 466);
  Serial.println(M5.Imu.isEnabled() ? "D09_READY imu=1" : "D09_READY imu=0");
  reportStatus();
}

static uint32_t lastFrame = 0;
static uint32_t lastReport = 0;

void loop() {
  M5.update();
  readTilt();

  // Snap-to-level with hysteresis: enter inside 1.0 degrees, leave
  // outside 1.5 — the edge never chatters.
  const float worst = fmaxf(fabsf(pitchDeg), fabsf(rollDeg));
  if (!isLevel && worst < LEVEL_TOLERANCE_DEG) {
    isLevel = true;
    M5.Speaker.tone(1046, 60, 0, true);
    M5.Power.setVibration(160); // the tick you feel when it's true
    vibeOffAt = millis() + 60;
  } else if (isLevel && worst > LEVEL_TOLERANCE_DEG * 1.5f) {
    isLevel = false;
  }

  updateVibe();
  if (Serial.available() && Serial.read() == 's') reportStatus();
  if (millis() - lastReport >= 2000) { lastReport = millis(); reportStatus(); }
  if (millis() - lastFrame >= 33) { // ~30 fps
    lastFrame = millis();
    render();
  }
  delay(2);
}
