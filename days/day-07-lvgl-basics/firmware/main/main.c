// ESPtember Day 07 — LVGL Basics
// Days 01–06 used LVGL as a renderer: put a thing on screen, leave it
// there. Today it becomes a UI toolkit. Three ideas carry the whole
// library: widgets are a tree, events connect touch to code, and
// screens are just root objects you can swap. Board bring-up
// (pmu_init, panel_reset_release) carries over from day 01.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"

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

// --- The app ------------------------------------------------------------
// Two screens. "Controls" holds one of each fundamental widget wired to
// a real effect; "About" exists to prove screens swap. Both are built
// once at startup and kept in memory — LVGL screens are cheap.

#define ESPTEMBER_ORANGE 0xff5b04

// Shared with the verification harness (test_harness.c): the harness
// injects taps through a virtual input device and reads this state back
// over USB — the same trick day 12 used to soak-test C25K.
lv_obj_t *day07_controls_screen;
lv_obj_t *day07_about_screen;
int day07_press_count = 0;
int day07_brightness = 100;
bool day07_orange_mode = false;
void day07_test_harness_init(void);

#define controls_screen day07_controls_screen
#define about_screen day07_about_screen

// Every interactive widget works the same way: create it, then attach a
// callback for the event you care about. The callback receives the
// event; lv_event_get_target() says which widget fired it.

// A button that counts its own presses. The count label rides along in
// the event's user_data — no globals needed.
static void count_button_cb(lv_event_t *e)
{
    lv_obj_t *label = lv_event_get_user_data(e);
    lv_label_set_text_fmt(label, "pressed %d", ++day07_press_count);
}

// A slider that drives real hardware: the panel's brightness command.
// LV_EVENT_VALUE_CHANGED fires continuously while dragging, so the
// panel tracks your finger.
static void brightness_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    day07_brightness = lv_slider_get_value(slider);
    bsp_display_brightness_set(day07_brightness);
}

// A switch that restyles the screen it lives on. Widgets are restyled
// live — no rebuild, the next frame just draws differently.
static void invert_switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    day07_orange_mode = lv_obj_has_state(sw, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(controls_screen,
                              day07_orange_mode ? lv_color_hex(ESPTEMBER_ORANGE)
                                                : lv_color_black(),
                              0);
}

// Screen swaps are one call. The animation is free.
static void go_about_cb(lv_event_t *e)
{
    (void)e;
    lv_screen_load_anim(about_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0,
                        false);
}

static void go_controls_cb(lv_event_t *e)
{
    (void)e;
    lv_screen_load_anim(controls_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0,
                        false);
}

// Small helper: a full-width button with a centered text label.
static lv_obj_t *make_button(lv_obj_t *parent, const char *text,
                             lv_event_cb_t cb, void *user_data)
{
    lv_obj_t *btn = lv_button_create(parent);
    // Finger-sized: 368px panel, ~64px tall targets.
    lv_obj_set_size(btn, lv_pct(100), 64);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return btn;
}

static void build_controls_screen(void)
{
    controls_screen = lv_obj_create(NULL); // NULL parent = a screen
    lv_obj_set_style_bg_color(controls_screen, lv_color_black(), 0);

    // A flex column: children stack top-to-bottom with even gaps.
    // Layout containers replace pixel math — this is how LVGL UIs
    // survive different screen sizes.
    lv_obj_t *col = lv_obj_create(controls_screen);
    lv_obj_set_size(col, lv_pct(94), lv_pct(100));
    lv_obj_center(col);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    // Start-aligned with a fixed gap: when the children outgrow the
    // container, LVGL makes it scrollable. Scrolling isn't a widget —
    // it's what any overflowing container does. Drag anywhere.
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col, 22, 0);
    lv_obj_set_style_pad_ver(col, 24, 0);
    // Bigger type for everything below — text styles inherit.
    lv_obj_set_style_text_font(controls_screen, &lv_font_montserrat_28, 0);

    lv_obj_t *title = lv_label_create(col);
    lv_label_set_text(title, "Day 07: LVGL Basics");
    lv_obj_set_style_text_color(title, lv_color_hex(ESPTEMBER_ORANGE), 0);

    // Button + its counter readout.
    lv_obj_t *count_label = lv_label_create(col);
    lv_label_set_text(count_label, "pressed 0");
    make_button(col, "Count", count_button_cb, count_label);
    // Move the readout under the button it reports on.
    lv_obj_move_to_index(count_label, lv_obj_get_index(count_label) + 1);

    // Brightness slider — 10..100 so the panel never goes fully dark
    // with no way to see the slider that rescues it.
    lv_obj_t *slider_label = lv_label_create(col);
    lv_label_set_text(slider_label, "Brightness");
    lv_obj_t *slider = lv_slider_create(col);
    lv_obj_set_size(slider, lv_pct(96), 24);
    lv_slider_set_range(slider, 10, 100);
    lv_slider_set_value(slider, 100, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, brightness_slider_cb, LV_EVENT_VALUE_CHANGED,
                        NULL);

    // Invert switch.
    lv_obj_t *switch_label = lv_label_create(col);
    lv_label_set_text(switch_label, "Orange mode");
    lv_obj_t *sw = lv_switch_create(col);
    lv_obj_set_size(sw, 100, 54);
    lv_obj_add_event_cb(sw, invert_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);

    make_button(col, "About " LV_SYMBOL_RIGHT, go_about_cb, NULL);
}

static void build_about_screen(void)
{
    about_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(about_screen, lv_color_black(), 0);

    lv_obj_set_style_text_font(about_screen, &lv_font_montserrat_28, 0);
    lv_obj_t *col = lv_obj_create(about_screen);
    lv_obj_set_size(col, lv_pct(90), lv_pct(80));
    lv_obj_center(col);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *text = lv_label_create(col);
    lv_label_set_text(text,
                      "A screen is just a widget with no parent.\n\n"
                      "This one slid in over the controls, which still "
                      "exist and keep their state.");
    // Wrap to the column instead of hand-placed line breaks.
    lv_obj_set_width(text, lv_pct(100));
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);

    make_button(col, LV_SYMBOL_LEFT " Back", go_controls_cb, NULL);
}

void app_main(void)
{
    pmu_init();
    panel_reset_release();
    bsp_display_start(); // also registers the CST816 touch controller
    bsp_display_backlight_on();

    // The LVGL task runs in the background; take the lock while building.
    bsp_display_lock(0);
    build_controls_screen();
    build_about_screen();
    lv_screen_load(controls_screen);
    bsp_display_unlock();

    day07_test_harness_init();
}
