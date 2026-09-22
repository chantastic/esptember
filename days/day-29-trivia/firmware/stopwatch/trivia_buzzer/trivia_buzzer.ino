// ESPtember Day 29 — Trivia, buzzer half (StopWatch)
// The capstone composes the month: ESP-NOW packets (day 20), button
// grammar (day 10), tones and haptics (day 10), and the fairness rule
// from the plan — the answer packet carries the *press timestamp*, not
// the arrival time, so radio latency can never decide a tie.
//
// The host (Waveshare 1.8) shows a statement; this half answers it:
// A = TRUE, B = FALSE. Feedback tells you instantly what the host
// ruled — a rising chirp and a buzz for right, a low tone for wrong.
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define TR_MAGIC 0x30565254u // "TRV0"
#define RF_CHANNEL 1

// type 0: question (host->all)  type 1: answer (player->host)
// type 2: verdict (host->player)
typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t type;
  uint8_t qid;
  uint8_t answer;  // 1 = true, 0 = false
  uint8_t correct; // verdicts: was the player right
  uint32_t press_ms; // player clock at the moment of the press
  char text[180];
} tr_msg_t;

static const uint8_t BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t hostMac[6];
static bool haveHost = false;

static char statement[180] = "waiting for host...";
static uint8_t currentQid = 0xFF;
static bool answered = false;
static int score = 0, asked = 0;
static volatile bool verdictPending = false;
static uint8_t verdictCorrect = 0;
static uint32_t vibeOffAt = 0;
static bool dirty = true;

static void onReceive(const esp_now_recv_info_t *info, const uint8_t *data,
                      int len) {
  if (len != sizeof(tr_msg_t)) return;
  tr_msg_t msg;
  memcpy(&msg, data, sizeof(msg));
  if (msg.magic != TR_MAGIC) return;
  if (!haveHost) {
    memcpy(hostMac, info->src_addr, 6);
    esp_now_peer_info_t reg = {};
    memcpy(reg.peer_addr, hostMac, 6);
    reg.channel = RF_CHANNEL;
    reg.ifidx = WIFI_IF_STA;
    esp_now_add_peer(&reg);
    haveHost = true;
  }
  if (msg.type == 0) { // new question
    msg.text[sizeof(msg.text) - 1] = 0;
    strncpy(statement, msg.text, sizeof(statement));
    currentQid = msg.qid;
    answered = false;
    asked++;
    dirty = true;
    Serial.printf("D29_Q qid=%u text=%s\n", msg.qid, statement);
  } else if (msg.type == 2 && msg.qid == currentQid) {
    verdictCorrect = msg.correct;
    verdictPending = true;
    if (msg.correct) score++;
    Serial.printf("D29_VERDICT qid=%u correct=%u score=%d\n", msg.qid,
                  msg.correct, score);
  }
}

static void sendAnswer(bool truthy) {
  if (!haveHost || answered || currentQid == 0xFF) return;
  answered = true;
  tr_msg_t msg = {TR_MAGIC, 1, currentQid, (uint8_t)(truthy ? 1 : 0), 0,
                  millis(), {0}};
  esp_now_send(hostMac, (uint8_t *)&msg, sizeof(msg));
  Serial.printf("D29_ANSWER qid=%u answer=%d\n", currentQid, (int)truthy);
  dirty = true;
}

// --- Display -------------------------------------------------------------
static void render() {
  M5.Display.startWrite();
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);

  M5.Display.setTextColor(0xFD20, TFT_BLACK);
  M5.Display.setTextSize(2);
  char head[24];
  snprintf(head, sizeof(head), "SCORE %d / %d", score, asked);
  M5.Display.drawString(head, 233, 60);

  // Wrap the statement by words across the face.
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  char buf[180];
  strncpy(buf, statement, sizeof(buf));
  int y = 140;
  char *word = strtok(buf, " ");
  char lineBuf[28] = "";
  while (word && y < 340) {
    if (strlen(lineBuf) + strlen(word) + 1 < 26) {
      if (lineBuf[0]) strlcat(lineBuf, " ", sizeof(lineBuf));
      strlcat(lineBuf, word, sizeof(lineBuf));
    } else {
      M5.Display.drawString(lineBuf, 233, y);
      y += 30;
      strlcpy(lineBuf, word, sizeof(lineBuf));
    }
    word = strtok(NULL, " ");
  }
  if (lineBuf[0]) M5.Display.drawString(lineBuf, 233, y);

  M5.Display.setTextSize(3);
  if (answered) {
    M5.Display.setTextColor(TFT_DARKGRAY, TFT_BLACK);
    M5.Display.drawString("locked in", 233, 390);
  } else if (currentQid != 0xFF) {
    M5.Display.setTextColor(0x07E0, TFT_BLACK);
    M5.Display.drawString("A TRUE", 130, 390);
    M5.Display.setTextColor(0xF800, TFT_BLACK);
    M5.Display.drawString("B FALSE", 336, 390);
  }
  M5.Display.endWrite();
}

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = true;
  cfg.internal_mic = false;
  cfg.internal_spk = true;
  cfg.fallback_board = m5::board_t::board_M5StopWatch;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(255);
  M5.Power.setVibration(0);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(RF_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_now_init();
  esp_now_register_recv_cb(onReceive);

  render();
  Serial.println("D29_READY buzzer");
}

void loop() {
  M5.update();
  const uint32_t now = millis();

  if (M5.BtnA.wasClicked()) sendAnswer(true);
  if (M5.BtnB.wasClicked()) sendAnswer(false);

  if (verdictPending) { // feedback from loop context, day 20's rule
    verdictPending = false;
    if (verdictCorrect) {
      M5.Speaker.tone(1046, 90, 0, true);
      M5.Power.setVibration(180);
      vibeOffAt = now + 120;
    } else {
      M5.Speaker.tone(220, 250, 0, true);
    }
    dirty = true;
  }
  if (vibeOffAt && int32_t(now - vibeOffAt) >= 0) {
    M5.Power.setVibration(0);
    vibeOffAt = 0;
  }

  // Serial test hooks: 'a'/'b' answer true/false, 's' status.
  if (Serial.available()) {
    const char c = Serial.read();
    if (c == 'a') sendAnswer(true);
    if (c == 'b') sendAnswer(false);
    if (c == 's')
      Serial.printf("D29_STATUS qid=%u answered=%d score=%d asked=%d\n",
                    currentQid, (int)answered, score, asked);
  }

  if (dirty) {
    dirty = false;
    render();
  }
  delay(10);
}
