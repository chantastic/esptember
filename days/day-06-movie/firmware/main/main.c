// ESPtember Day 06 — A Movie
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


#include <string.h>
#include "esp_partition.h"
#include "src/misc/cache/instance/lv_image_cache.h"

#define MOVIE_WIDTH 184
#define MOVIE_HEIGHT 224
#define MOVIE_FPS 10
#define FRAME_BYTES (MOVIE_WIDTH * MOVIE_HEIGHT * 2)

static lv_image_dsc_t frame;

void app_main(void)
{
    board_start();
    const esp_partition_t *media = esp_partition_find_first(0x40, 0, "media");
    if (!media || MOVIE_FLASH_BYTES > media->size || MOVIE_FLASH_BYTES % FRAME_BYTES) {
        ESP_LOGE("media", "Invalid flash movie size or partition");
        abort();
    }
    FILE *sd_file = NULL;
    size_t movie_bytes = MOVIE_FLASH_BYTES;
    esp_err_t sd_result = bsp_sdcard_mount();
    if (sd_result == ESP_OK) {
        sd_file = fopen(BSP_SD_MOUNT_POINT "/movie.rgb", "rb");
        if (sd_file) {
            if (fseek(sd_file, 0, SEEK_END) != 0) {
                fclose(sd_file);
                sd_file = NULL;
            } else {
                long size = ftell(sd_file);
                if (size <= 0 || size % FRAME_BYTES != 0 || fseek(sd_file, 0, SEEK_SET) != 0) {
                    ESP_LOGW("media", "SD movie must contain complete 184x224 RGB565 frames");
                    fclose(sd_file);
                    sd_file = NULL;
                } else {
                    movie_bytes = (size_t)size;
                }
            }
        }
        if (!sd_file) ESP_LOGW("media", "No valid /sdcard/movie.rgb; using flash demo");
    } else {
        ESP_LOGW("media", "SD mount: %s; using flash demo", esp_err_to_name(sd_result));
    }

    uint8_t *staging = heap_caps_malloc(FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    uint8_t *visible = heap_caps_calloc(1, FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!staging || !visible) {
        ESP_LOGE("media", "Cannot allocate two %u-byte frame buffers", FRAME_BYTES);
        abort();
    }
    frame = (lv_image_dsc_t) {
        .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                    .w = MOVIE_WIDTH, .h = MOVIE_HEIGHT, .stride = MOVIE_WIDTH * 2 },
        .data_size = FRAME_BYTES, .data = visible,
    };
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
    lv_obj_t *image = lv_image_create(lv_screen_active());
    lv_image_set_src(image, &frame);
    lv_obj_center(image);
    bsp_display_unlock();

    const size_t frame_count = movie_bytes / FRAME_BYTES;
    ESP_LOGI("media", "READY day06: source=%s bytes=%u frames=%u target=%dfps",
             sd_file ? "SD" : "flash", (unsigned)movie_bytes, (unsigned)frame_count, MOVIE_FPS);
    TickType_t deadline = xTaskGetTickCount();
    size_t index = 0;
    unsigned frames = 0, late = 0;
    int64_t report_at = esp_timer_get_time();
    while (1) {
        // Read outside the display lock: the UI can keep drawing the previous frame.
        bool ok = sd_file ? fread(staging, 1, FRAME_BYTES, sd_file) == FRAME_BYTES
                          : esp_partition_read(media, index * FRAME_BYTES, staging, FRAME_BYTES) == ESP_OK;
        if (!ok) {
            ESP_LOGE("media", "Movie read failed at frame %u", (unsigned)index);
            break;
        }
        // LVGL must never draw a frame while we are overwriting its pixels.
        bsp_display_lock(0);
        lv_image_cache_drop(&frame);
        memcpy(visible, staging, FRAME_BYTES);
        lv_obj_invalidate(image);
        bsp_display_unlock();
        if (++index == frame_count) {
            index = 0;
            if (sd_file && fseek(sd_file, 0, SEEK_SET) != 0) {
                ESP_LOGE("media", "SD rewind failed");
                break;
            }
        }
        frames++;
        if (xTaskDelayUntil(&deadline, pdMS_TO_TICKS(1000 / MOVIE_FPS)) == pdFALSE) late++;
        int64_t now = esp_timer_get_time();
        if (now - report_at >= 10000000) {
            ESP_LOGI("media", "MOVIE source=%s submitted_fps=%.2f late=%u heap=%u psram=%u",
                     sd_file ? "SD" : "flash", frames * 1000000.0 / (now - report_at), late,
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
            frames = late = 0;
            report_at = now;
        }
    }
    if (sd_file) fclose(sd_file);
    free(staging);
    // The displayed image still owns a reference to visible; retain that buffer.
    bsp_display_lock(0);
    lv_obj_t *error = lv_label_create(lv_screen_active());
    lv_label_set_text(error, "Movie read failed");
    lv_obj_align(error, LV_ALIGN_BOTTOM_MID, 0, -24);
    bsp_display_unlock();
    heartbeat("day06 stopped");
}
