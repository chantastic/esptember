// ESPtember Day 04 — A Carousel
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

extern const uint8_t circle_start[] asm("_binary_circle_rgb565_start");
static const lv_image_dsc_t circle = {
    .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                .w = 240, .h = 240, .stride = 480 },
    .data_size = 240 * 240 * 2, .data = circle_start,
};
extern const uint8_t square_start[] asm("_binary_square_rgb565_start");
static const lv_image_dsc_t square = {
    .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                .w = 240, .h = 240, .stride = 480 },
    .data_size = 240 * 240 * 2, .data = square_start,
};
extern const uint8_t triangle_start[] asm("_binary_triangle_rgb565_start");
static const lv_image_dsc_t triangle = {
    .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                .w = 240, .h = 240, .stride = 480 },
    .data_size = 240 * 240 * 2, .data = triangle_start,
};

static void slide_changed(lv_event_t *event)
{
    lv_obj_t *carousel = lv_event_get_target(event);
    lv_obj_t *tile = lv_tileview_get_tile_active(carousel);
    ESP_LOGI("media", "SWIPE slide=%ld", (long)lv_obj_get_index(tile) + 1);
}

void app_main(void)
{
    board_start();
    const lv_image_dsc_t *slides[] = { &circle, &square, &triangle };
    bsp_display_lock(0);
    lv_obj_t *carousel = lv_tileview_create(lv_screen_active());
    lv_obj_set_size(carousel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(carousel, 0, 0);
    lv_obj_set_style_border_width(carousel, 0, 0);
    lv_obj_set_style_bg_color(carousel, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(carousel, LV_SCROLLBAR_MODE_OFF);
    for (uint8_t i = 0; i < 3; i++) {
        lv_dir_t directions = LV_DIR_NONE;
        if (i > 0) directions |= LV_DIR_LEFT;
        if (i < 2) directions |= LV_DIR_RIGHT;
        lv_obj_t *tile = lv_tileview_add_tile(carousel, i, 0, directions);
        lv_obj_set_style_pad_all(tile, 0, 0);
        lv_obj_t *image = lv_image_create(tile);
        lv_image_set_src(image, slides[i]);
        lv_obj_center(image);
    }
    lv_obj_add_event_cb(carousel, slide_changed, LV_EVENT_VALUE_CHANGED, NULL);
    bsp_display_unlock();
    ESP_LOGI("media", "READY day04: swipe between orange circle, square, triangle");
    heartbeat("day04");
}
