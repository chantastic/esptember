// ESPtember Day 01 — Hello World
// Print text on the Waveshare ESP32-S3-Touch-AMOLED-1.8's screen, the
// most naive way possible — plus the two pieces of board bring-up the
// BSP is missing (PMU power budget, V2 panel reset) without which the
// board dies minutes in. This is a V2 board: CO5300 panel driver,
// CST816-family touch, AMOLED over QSPI, AXP2101 PMU.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"

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
// ------------------------------------------------------------------------

void app_main(void)
{
    pmu_init();
    panel_reset_release();
    bsp_display_start();
    // AMOLEDs have no backlight — this sends the panel its brightness
    // command. Required: the one sent during init doesn't stick.
    bsp_display_backlight_on();

    bsp_display_lock(0);
    // Not styling — physics: the dark theme's background is dark grey,
    // which keeps every AMOLED pixel lit. True black (#000000) turns
    // them off.
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello, ESPtember!");
    // Not styling — survival: at the default position (top-left, 0,0)
    // the label hides under the panel's rounded corner entirely.
    lv_obj_center(label);
    bsp_display_unlock();

    // app_main can simply return — the BSP's LVGL task keeps the
    // screen alive without us.
}
