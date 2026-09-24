// ESPtember Day 30 — Trivia, host half (Waveshare 1.8)
// The host owns the deck and the truth: it broadcasts each statement
// over ESP-NOW, receives answers stamped with the player's press time,
// rules them, and returns verdicts. Tap the screen to advance. Twelve
// original statements bake into flash — no network, no API, no
// accounts; the sequel wires Open Trivia DB through day 22's portal.
// Board bring-up (pmu_init, panel_reset_release) carries over from
// day 01.
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"

#define TR_MAGIC 0x30565254u
#define RF_CHANNEL 1

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t type; // 0 question, 1 answer, 2 verdict
    uint8_t qid;
    uint8_t answer;
    uint8_t correct;
    uint32_t press_ms;
    char text[180];
} tr_msg_t;

// --- The deck ------------------------------------------------------------
// Original statements; answers verified while writing them.
typedef struct {
    const char *text;
    bool truth;
} question_t;

static const question_t DECK[] = {
    {"The ESP32-S3 has two CPU cores", true},
    {"Morse code for SOS is dot dot dot dash dash dash dot dot dot", true},
    {"Bluetooth Classic works on the ESP32-S3", false},
    {"A dah lasts three times as long as a dit", true},
    {"AMOLED screens need a backlight", false},
    {"ESP-NOW frames can carry up to 1500 bytes", false},
    {"The first Tamagotchi shipped in 1996", true},
    {"16-bit audio has a dynamic range of about 96 dB", true},
    {"Wi-Fi channel 14 is legal everywhere", false},
    {"PSRAM on this board is larger than its internal RAM", true},
    {"A captive portal works by hijacking HTTP", false},
    {"At 100 Hz tick rate, a 5 ms FreeRTOS delay rounds to zero", true},
};
#define DECK_SIZE (sizeof(DECK) / sizeof(DECK[0]))

// --- AXP2101 power management (day 01, unchanged) ----------------------
#define AXP2101_ADDR 0x34

static i2c_master_dev_handle_t pmu;

static uint8_t pmu_read(uint8_t reg)
{
    uint8_t val = 0;
    i2c_master_transmit_receive(pmu, &reg, 1, &val, 1, 100);
    return val;
}

static void pmu_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    i2c_master_transmit(pmu, buf, 2, 100);
}

static void pmu_init(void)
{
    bsp_i2c_init();
    i2c_device_config_t cfg = {
        .device_address = AXP2101_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(bsp_i2c_get_handle(), &cfg, &pmu));
    pmu_write(0x16, (pmu_read(0x16) & 0xF8) | 0x02);
    pmu_write(0x62, (pmu_read(0x62) & 0xE0) | 0x09);
}

// --- V2 panel reset (day 01, unchanged) --------------------------------
#include "esp_io_expander_tca9554.h"

static void panel_reset_release(void)
{
    esp_io_expander_handle_t expander = NULL;
    ESP_ERROR_CHECK(esp_io_expander_new_i2c_tca9554(
        bsp_i2c_get_handle(), ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000,
        &expander));
    const uint32_t reset_pins =
        IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1 | IO_EXPANDER_PIN_NUM_2;
    ESP_ERROR_CHECK(
        esp_io_expander_set_dir(expander, reset_pins, IO_EXPANDER_OUTPUT));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander, reset_pins, 1));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander, reset_pins, 0));
    vTaskDelay(pdMS_TO_TICKS(300));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander, reset_pins, 1));
}

// --- Game state ----------------------------------------------------------
static int qIndex = -1; // -1: lobby
static int playerScore = 0;
static bool answerIn = false;
static uint8_t lastAnswer = 0, lastCorrect = 0;
static uint32_t lastPressMs = 0;

static lv_obj_t *title_label;
static lv_obj_t *statement_label;
static lv_obj_t *result_label;
static lv_obj_t *hint_label;

static const uint8_t BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static void broadcast_question(void)
{
    tr_msg_t msg = {TR_MAGIC, 0, (uint8_t)qIndex, 0, 0, 0, {0}};
    strncpy(msg.text, DECK[qIndex].text, sizeof(msg.text) - 1);
    esp_now_send(BROADCAST, (const uint8_t *)&msg, sizeof(msg));
    printf("D29_ASK qid=%d text=%s\n", qIndex, DECK[qIndex].text);
}

static void ui_update(void)
{
    bsp_display_lock(0);
    if (qIndex < 0) {
        lv_label_set_text(title_label, "TRIVIA");
        lv_label_set_text(statement_label,
                          "The buzzer answers\nA true / B false.");
        lv_label_set_text(result_label, "");
        lv_label_set_text(hint_label, "tap to start");
    } else {
        lv_label_set_text_fmt(title_label, "Q%d / %d    score %d", qIndex + 1,
                              (int)DECK_SIZE, playerScore);
        lv_label_set_text(statement_label, DECK[qIndex].text);
        if (answerIn)
            lv_label_set_text_fmt(
                result_label, "%s  (answered %s at +%lums)",
                lastCorrect ? "CORRECT" : "WRONG",
                lastAnswer ? "TRUE" : "FALSE",
                (unsigned long)lastPressMs);
        else
            lv_label_set_text(result_label, "waiting for buzz...");
        lv_label_set_text(hint_label,
                          qIndex + 1 < (int)DECK_SIZE ? "tap for next"
                                                      : "tap to finish");
    }
    bsp_display_unlock();
}

static void on_receive(const esp_now_recv_info_t *info, const uint8_t *data,
                       int len)
{
    if (len != sizeof(tr_msg_t)) return;
    tr_msg_t msg;
    memcpy(&msg, data, sizeof(msg));
    if (msg.magic != TR_MAGIC || msg.type != 1) return;
    if (msg.qid != qIndex || answerIn) return; // stale or duplicate

    answerIn = true;
    lastAnswer = msg.answer;
    lastPressMs = msg.press_ms;
    lastCorrect = (msg.answer == (DECK[qIndex].truth ? 1 : 0));
    if (lastCorrect) playerScore++;
    printf("D29_RULED qid=%u answer=%u correct=%u press_ms=%lu score=%d\n",
           msg.qid, msg.answer, lastCorrect, (unsigned long)msg.press_ms,
           playerScore);

    // Verdict back to the player (register them on first contact).
    esp_now_peer_info_t reg = {.channel = RF_CHANNEL, .ifidx = WIFI_IF_STA};
    memcpy(reg.peer_addr, info->src_addr, 6);
    esp_now_add_peer(&reg); // idempotent-ish: EXIST error is fine
    tr_msg_t verdict = {TR_MAGIC, 2, msg.qid, msg.answer, lastCorrect, 0, {0}};
    esp_now_send(info->src_addr, (const uint8_t *)&verdict, sizeof(verdict));
    ui_update();
}

static void advance(void)
{
    if (qIndex >= 0 && qIndex + 1 >= (int)DECK_SIZE) { // game over -> lobby
        bsp_display_lock(0);
        lv_label_set_text_fmt(statement_label, "Final score\n%d / %d",
                              playerScore, (int)DECK_SIZE);
        lv_label_set_text(result_label, "");
        lv_label_set_text(hint_label, "tap for a new game");
        bsp_display_unlock();
        printf("D29_FINAL score=%d of=%d\n", playerScore, (int)DECK_SIZE);
        qIndex = -1;
        playerScore = 0;
        return;
    }
    qIndex++;
    answerIn = false;
    broadcast_question();
    ui_update();
}

static void screen_cb(lv_event_t *e)
{
    (void)e;
    advance();
}

static void build_ui(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_text_font(screen, &lv_font_montserrat_28, 0);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, screen_cb, LV_EVENT_CLICKED, NULL);

    title_label = lv_label_create(screen);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xff5b04), 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 24);

    statement_label = lv_label_create(screen);
    lv_obj_set_width(statement_label, lv_pct(90));
    lv_obj_set_style_text_align(statement_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(statement_label, LV_ALIGN_CENTER, 0, -40);

    result_label = lv_label_create(screen);
    lv_obj_set_width(result_label, lv_pct(90));
    lv_obj_set_style_text_align(result_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(result_label, lv_color_hex(0x00ff00), 0);
    lv_obj_align(result_label, LV_ALIGN_CENTER, 0, 80);

    hint_label = lv_label_create(screen);
    lv_obj_set_style_text_color(hint_label, lv_color_hex(0x888888), 0);
    lv_obj_align(hint_label, LV_ALIGN_BOTTOM_MID, 0, -20);
}

static void espnow_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(RF_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_receive));
    esp_now_peer_info_t bc = {.channel = RF_CHANNEL, .ifidx = WIFI_IF_STA};
    memcpy(bc.peer_addr, BROADCAST, 6);
    ESP_ERROR_CHECK(esp_now_add_peer(&bc));
}

// Serial test hooks: 'n' advances (lobby -> Q1 -> ... -> final).
static void console_task(void *arg)
{
    (void)arg;
    while (1) {
        const int c = getchar();
        if (c == 'n') advance();
        else if (c == 's')
            printf("D29_STATUS q=%d score=%d answered=%d\n", qIndex,
                   playerScore, (int)answerIn);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void)
{
    pmu_init();
    panel_reset_release();
    bsp_display_start();
    bsp_display_backlight_on();
    bsp_display_lock(0);
    build_ui();
    bsp_display_unlock();
    espnow_init();
    ui_update();
    xTaskCreate(console_task, "console", 4096, NULL, 5, NULL);
    printf("D29_READY host deck=%d\n", (int)DECK_SIZE);
}
