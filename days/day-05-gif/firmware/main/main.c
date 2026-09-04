// ESPtember Day 05 — A GIF
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"

// --- AXP2101 power management ------------------------------------------
// The BSP doesn't configure the board's AXP2101 PMU at all. Its default
// VBUS input limit is 500 mA — once battery charging ramps up, the
// ESP32-S3 + AMOLED + charger exceed that and the PMU cuts system power
// (screen freezes/blanks, USB drops, ~1–4 min in). Raise the input
// limit and cap the charge current so the budget always fits.

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
    // 0x16 bits[2:0]: VBUS input current limit — 2 = 900 mA
    pmu_write(0x16, (pmu_read(0x16) & 0xF8) | 0x02);
    // 0x62 bits[4:0]: battery charge current — 9 = 300 mA
    pmu_write(0x62, (pmu_read(0x62) & 0xE0) | 0x09);
}
// --- V2 panel reset --------------------------------------------------
// On V2 boards (CO5300 panel, CST816-family touch) the panel's reset
// line sits behind a TCA9554 I/O expander that neither the BSP nor the
// examples ever drive. Left floating, the panel may come up — and then
// randomly drop dark minutes later while every esp_lcd call still
// returns ESP_OK. Pulse reset like the factory firmware does, then
// hold it high. Must run BEFORE bsp_display_start().
// https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8/issues/12

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

static void board_start(void)
{
    pmu_init();
    panel_reset_release();
    if (!bsp_display_start()) abort();
    ESP_ERROR_CHECK(bsp_display_backlight_on());
}

static void heartbeat(const char *lesson)
{
    while (1) {
        ESP_LOGI("media", "%s alive: uptime=%llds heap=%u psram=%u", lesson,
                 esp_timer_get_time() / 1000000,
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

extern const uint8_t gif_start[] asm("_binary_orbit_gif_start");
extern const uint8_t gif_end[] asm("_binary_orbit_gif_end");
static lv_image_dsc_t esptember_loop;
static unsigned loops;

static void loop_finished(lv_event_t *event)
{
    (void)event;
    ESP_LOGI("media", "GIF loop=%u", ++loops);
}

void app_main(void)
{
    board_start();
    esptember_loop = (lv_image_dsc_t) {
        .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RAW,
                    .w = 160, .h = 160 },
        .data_size = gif_end - gif_start, .data = gif_start,
    };
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
    lv_obj_t *animation = lv_gif_create(lv_screen_active());
    lv_gif_set_src(animation, &esptember_loop);
    if (lv_gif_is_loaded(animation)) {
        lv_gif_set_loop_count(animation, 0);
        lv_obj_center(animation);
        lv_obj_add_event_cb(animation, loop_finished, LV_EVENT_READY, NULL);
        ESP_LOGI("media", "READY day05: GIF loaded; bytes=%lu; ARGB8888 canvas=102400 bytes",
                 (unsigned long)esptember_loop.data_size);
    } else {
        ESP_LOGE("media", "GIF load failed");
        lv_obj_t *error = lv_label_create(lv_screen_active());
        lv_label_set_text(error, "GIF failed to load");
        lv_obj_center(error);
    }
    bsp_display_unlock();
    heartbeat("day05");
}
