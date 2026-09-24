// ESPtember Day 21 — Yo (StopWatch half)
// ESP-NOW is peer-to-peer Wi-Fi with no router, no credentials, no
// TCP — just MAC addresses and 250-byte frames. This half runs on the
// M5 StopWatch; its counterpart (../waveshare) runs on the Waveshare
// AMOLED 1.8 under ESP-IDF. Different brands, different frameworks,
// same 24 bytes on the air: the protocol is the only contract.
//
// The app is the app "Yo": broadcast hello -> roster of nearby boards;
// pick one, press the pusher, their board buzzes with your name.
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// --- The wire protocol (shared with the ESP-IDF half, byte for byte) ---
#define YO_MAGIC 0x30594F45u // "EOY0"
#define YO_CHANNEL 1

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t type; // 0 = hello, 1 = poke
  char name[12];
} yo_msg_t;

static const char *MY_NAME = "StopWatch";

// --- Roster -------------------------------------------------------------
typedef struct {
  uint8_t mac[6];
  char name[12];
  uint32_t lastSeen;
} peer_t;

#define MAX_PEERS 8
static peer_t peers[MAX_PEERS];
static int peerCount = 0;
static int selected = 0;

static peer_t *findPeer(const uint8_t *mac) {
  for (int i = 0; i < peerCount; i++)
    if (!memcmp(peers[i].mac, mac, 6)) return &peers[i];
  return NULL;
}

// --- Receive ----------------------------------------------------------------
static char lastPokeFrom[12] = "";
static uint32_t pokeShownAt = 0;
static volatile bool pokePending = false;
static bool dirty = true;

static void onReceive(const esp_now_recv_info_t *info, const uint8_t *data,
                      int len) {
  if (len != sizeof(yo_msg_t)) return;
  yo_msg_t msg;
  memcpy(&msg, data, sizeof(msg));
  if (msg.magic != YO_MAGIC) return;
  msg.name[sizeof(msg.name) - 1] = 0;

  peer_t *p = findPeer(info->src_addr);
  if (!p && peerCount < MAX_PEERS) {
    p = &peers[peerCount++];
    memcpy(p->mac, info->src_addr, 6);
    // Unicast needs a registered peer; broadcast reception doesn't.
    esp_now_peer_info_t reg = {};
    memcpy(reg.peer_addr, info->src_addr, 6);
    reg.channel = YO_CHANNEL;
    reg.ifidx = WIFI_IF_STA;
    esp_now_add_peer(&reg);
    Serial.printf("D20_PEER %02x:%02x:%02x:%02x:%02x:%02x %s\n",
                  info->src_addr[0], info->src_addr[1], info->src_addr[2],
                  info->src_addr[3], info->src_addr[4], info->src_addr[5],
                  msg.name);
  }
  if (!p) return;
  strncpy(p->name, msg.name, sizeof(p->name));
  p->lastSeen = millis();

  if (msg.type == 1) { // poke! Feedback fires from loop(), not from
    // this Wi-Fi-task callback — I2C from two tasks is a race.
    strncpy(lastPokeFrom, msg.name, sizeof(lastPokeFrom));
    pokePending = true;
    Serial.printf("D20_POKED by=%s\n", msg.name);
  }
  dirty = true;
}

static void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  (void)info;
  // The callback reports link-layer delivery: sent != received unless
  // this says success.
  Serial.printf("D20_SENT %s\n",
                status == ESP_NOW_SEND_SUCCESS ? "acked" : "lost");
}

// --- Send ----------------------------------------------------------------------
static const uint8_t BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static void sendHello() {
  yo_msg_t msg = {YO_MAGIC, 0, {0}};
  strncpy(msg.name, MY_NAME, sizeof(msg.name));
  esp_now_send(BROADCAST, (uint8_t *)&msg, sizeof(msg));
}

static void sendPoke(int index) {
  if (index >= peerCount) return;
  yo_msg_t msg = {YO_MAGIC, 1, {0}};
  strncpy(msg.name, MY_NAME, sizeof(msg.name));
  esp_now_send(peers[index].mac, (uint8_t *)&msg, sizeof(msg));
  Serial.printf("D20_POKE to=%s\n", peers[index].name);
}

// --- Display ---------------------------------------------------------------------
static void render() {
  M5.Display.startWrite();
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);

  if (millis() - pokeShownAt < 2500 && lastPokeFrom[0]) {
    M5.Display.setTextColor(0x07E0, TFT_BLACK);
    M5.Display.setTextSize(4);
    M5.Display.drawString("YO!", 233, 180);
    M5.Display.setTextSize(3);
    M5.Display.drawString(lastPokeFrom, 233, 260);
    M5.Display.endWrite();
    return;
  }

  M5.Display.setTextColor(0xFD20, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.drawString("YO", 233, 70);

  M5.Display.setTextSize(3);
  if (!peerCount) {
    M5.Display.setTextColor(TFT_DARKGRAY, TFT_BLACK);
    M5.Display.drawString("listening...", 233, 220);
  }
  const uint32_t now = millis();
  for (int i = 0; i < peerCount; i++) {
    const bool alive = now - peers[i].lastSeen < 10000;
    M5.Display.setTextColor(i == selected ? TFT_WHITE : TFT_DARKGRAY,
                            TFT_BLACK);
    char line[24];
    snprintf(line, sizeof(line), "%s%s %s", i == selected ? "> " : "  ",
             peers[i].name, alive ? "" : "(gone)");
    M5.Display.drawString(line, 233, 160 + i * 44);
  }

  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  M5.Display.drawString("A: select   B: YO", 233, 390);
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

  // ESP-NOW rides the Wi-Fi radio in station mode with no connection —
  // both halves pin channel 1 so they can hear each other.
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(YO_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_now_init();
  esp_now_register_recv_cb(onReceive);
  esp_now_register_send_cb(onSent);
  esp_now_peer_info_t bc = {};
  memcpy(bc.peer_addr, BROADCAST, 6);
  bc.channel = YO_CHANNEL;
  bc.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&bc);

  render();
  Serial.printf("D20_READY name=%s mac=%s\n", MY_NAME,
                WiFi.macAddress().c_str());
}

static uint32_t lastHello = 0;
static uint32_t vibeOffAt = 0;

void loop() {
  M5.update();
  const uint32_t now = millis();

  if (now - lastHello >= 3000) {
    lastHello = now;
    sendHello();
    dirty = true; // refresh (gone) markers
  }

  if (M5.BtnA.wasClicked() && peerCount) {
    selected = (selected + 1) % peerCount;
    dirty = true;
  }
  if (M5.BtnB.wasClicked()) sendPoke(selected);

  // Serial test hooks: 'p' pokes the selected peer, 's' reports.
  if (Serial.available()) {
    const char c = Serial.read();
    if (c == 'p') sendPoke(selected);
    if (c == 's')
      Serial.printf("D20_STATUS peers=%d selected=%d\n", peerCount, selected);
  }

  if (pokePending) {
    pokePending = false;
    pokeShownAt = now;
    M5.Speaker.tone(1046, 200, 0, true);
    M5.Power.setVibration(255);
    vibeOffAt = now + 250;
    dirty = true;
  }
  if (vibeOffAt && int32_t(now - vibeOffAt) >= 0) {
    M5.Power.setVibration(0);
    vibeOffAt = 0;
  }

  if (dirty || (pokeShownAt && now - pokeShownAt < 2600)) {
    dirty = false;
    render();
  }
  delay(10);
}
