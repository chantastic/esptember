// ESPtember Day 01 — Hello World
// Put text on the Waveshare ESP32-S3-Touch-AMOLED-1.8's screen
// (SH8601 AMOLED over QSPI, powered by the AXP2101 PMU — all
// handled by the board support package), and say hello over USB too.
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"

void app_main(void)
{
    bsp_display_start();
    bsp_display_backlight_on();

    bsp_display_lock(0);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello, ESPtember!");
    lv_obj_set_style_text_color(label, lv_color_hex(0xff5b04), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label, lv_pct(90));
    lv_obj_center(label);
    bsp_display_unlock();

    while (true) {
        printf("Hello, ESPtember!\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
