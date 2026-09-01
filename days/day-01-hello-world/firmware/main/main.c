// ESPtember Day 01 — Hello World
// Print text on the Waveshare ESP32-S3-Touch-AMOLED-1.8's screen,
// the most naive way possible. The board support package brings up
// the display (SH8601 AMOLED over QSPI, AXP2101 PMU) and hands us
// LVGL; we put one label on it and we're done.
#include "bsp/esp-bsp.h"
#include "lvgl.h"

void app_main(void)
{
    bsp_display_start();
    bsp_display_backlight_on();

    bsp_display_lock(0);
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello, ESPtember!");
    lv_obj_center(label);
    bsp_display_unlock();

    // app_main can simply return — the BSP's LVGL task keeps the
    // screen alive without us.
}
