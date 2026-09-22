// ESPtember Day 14 — Sound Level (dB)
// Day 08 sent audio out; today audio comes in. The onboard microphone
// feeds an RMS meter: a big dB readout, a live bar, and a peak-hold
// marker. Honesty first: a MEMS mic with no calibration measures
// dBFS — decibels relative to the loudest sample the converter can
// represent — not absolute SPL. The relative motion is real; the
// absolute number would need a reference meter. Board bring-up
// (pmu_init, panel_reset_release) carries over from day 01.
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "esp_codec_dev.h"

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

// --- Metering ---------------------------------------------------------------
// 16-bit mono at 16 kHz, measured in 50 ms windows: 800 samples per
// reading, 20 readings per second. RMS is the physically meaningful
// average for acoustic energy — peak would jump on every click, mean
// would cancel to zero.

#define SAMPLE_RATE 16000
#define WINDOW_SAMPLES (SAMPLE_RATE / 20)

static int16_t window[WINDOW_SAMPLES];

// Shared with the UI timer (written by mic task, read by LVGL tick).
static volatile float current_db = -96.0f;
static volatile float peak_db = -96.0f;
static volatile uint32_t peak_at = 0;

static void mic_task(void *arg)
{
    esp_codec_dev_handle_t mic = (esp_codec_dev_handle_t)arg;
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = SAMPLE_RATE,
        .channel = 1,
        .bits_per_sample = 16,
    };
    esp_codec_dev_open(mic, &fs);
    esp_codec_dev_set_in_gain(mic, 30.0);
    int report = 0;
    while (1) {
        esp_codec_dev_read(mic, window, sizeof(window));
        double sum = 0;
        for (int i = 0; i < WINDOW_SAMPLES; i++)
            sum += (double)window[i] * window[i];
        float rms = sqrtf(sum / WINDOW_SAMPLES);
        // dBFS: 0 dB is a full-scale sine; silence clamps at -96.
        float db = rms > 0.5f ? 20.0f * log10f(rms / 32768.0f) : -96.0f;
        current_db = db;
        if (db > peak_db || xTaskGetTickCount() - peak_at > pdMS_TO_TICKS(3000)) {
            peak_db = db; // hold peaks three seconds, then follow down
            peak_at = xTaskGetTickCount();
        }
        if (++report % 20 == 0)
            printf("D14_LEVEL db=%.1f peak=%.1f\n", db, peak_db);
    }
}

// --- UI -----------------------------------------------------------------------
#define ESPTEMBER_ORANGE 0xff5b04
#define DB_FLOOR -80.0f // display range: -80 dBFS to 0 dBFS

static lv_obj_t *db_label;
static lv_obj_t *bar;
static lv_obj_t *peak_line;
static lv_obj_t *bar_area;

static int db_to_pct(float db)
{
    float pct = (db - DB_FLOOR) * 100.0f / -DB_FLOOR;
    return pct < 0 ? 0 : pct > 100 ? 100 : (int)pct;
}

static void meter_tick(lv_timer_t *t)
{
    (void)t;
    lv_label_set_text_fmt(db_label, "%d", (int)current_db);
    lv_bar_set_value(bar, db_to_pct(current_db), LV_ANIM_OFF);
    // Peak marker rides the bar's width.
    int width = lv_obj_get_width(bar_area);
    lv_obj_set_x(peak_line, width * db_to_pct(peak_db) / 100);
}

static void build_ui(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_text_font(screen, &lv_font_montserrat_28, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "SOUND LEVEL");
    lv_obj_set_style_text_color(title, lv_color_hex(ESPTEMBER_ORANGE), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 28);

    // The big number: dBFS, so it lives between -96 and 0.
    db_label = lv_label_create(screen);
    lv_obj_set_style_text_font(db_label, &lv_font_montserrat_48, 0);
    lv_label_set_text(db_label, "-96");
    lv_obj_align(db_label, LV_ALIGN_CENTER, -20, -60);

    lv_obj_t *unit = lv_label_create(screen);
    lv_label_set_text(unit, "dBFS");
    lv_obj_set_style_text_color(unit, lv_color_hex(0x888888), 0);
    lv_obj_align_to(unit, db_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 8, 0);

    // Bar + peak-hold marker.
    bar_area = lv_obj_create(screen);
    lv_obj_set_size(bar_area, lv_pct(86), 70);
    lv_obj_align(bar_area, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_opa(bar_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar_area, 0, 0);
    lv_obj_set_style_pad_all(bar_area, 0, 0);

    bar = lv_bar_create(bar_area);
    lv_obj_set_size(bar, lv_pct(100), 36);
    lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 0, 16);
    lv_bar_set_range(bar, 0, 100);
    lv_obj_set_style_bg_color(bar, lv_color_hex(ESPTEMBER_ORANGE),
                              LV_PART_INDICATOR);

    peak_line = lv_obj_create(bar_area);
    lv_obj_set_size(peak_line, 4, 68);
    lv_obj_set_style_bg_color(peak_line, lv_color_white(), 0);
    lv_obj_set_style_border_width(peak_line, 0, 0);
    lv_obj_set_style_radius(peak_line, 0, 0);
    lv_obj_align(peak_line, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *note = lv_label_create(screen);
    lv_label_set_text(note, "relative, uncalibrated");
    lv_obj_set_style_text_color(note, lv_color_hex(0x666666), 0);
    lv_obj_align(note, LV_ALIGN_BOTTOM_MID, 0, -26);

    lv_timer_create(meter_tick, 50, NULL); // match the metering rate
}

void app_main(void)
{
    pmu_init();
    panel_reset_release();

    bsp_audio_init(NULL);
    esp_codec_dev_handle_t mic = bsp_audio_codec_microphone_init();
    xTaskCreate(mic_task, "mic", 4096, mic, 6, NULL);

    bsp_display_start();
    bsp_display_backlight_on();

    bsp_display_lock(0);
    build_ui();
    bsp_display_unlock();
}
