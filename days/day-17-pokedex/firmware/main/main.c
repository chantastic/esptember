// ESPtember Day 17 — Pokedex
// The asset pipeline at scale. Days 03-05 converted one image by hand;
// today a build-time script fetches 151 sprites and their species data
// from PokeAPI and generates 2.7 MB of C arrays — nothing committed,
// nothing hand-drawn, one command. The UI is the list/detail pattern
// every data app uses: scroll the index, tap for the entry. Board
// bring-up (pmu_init, panel_reset_release) carries over from day 01.
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "dex.h"

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

// --- Screens ----------------------------------------------------------------
#define ESPTEMBER_ORANGE 0xff5b04

static lv_obj_t *list_screen;
static lv_obj_t *detail_screen;

// The detail screen is built once and *re-dressed* per entry — no
// rebuild, no allocation churn while browsing.
static lv_obj_t *detail_sprite;
static lv_obj_t *detail_name;
static lv_obj_t *detail_types;
static lv_obj_t *detail_body;
static lv_obj_t *detail_stats[4];
static lv_image_dsc_t sprite_dsc;

static const char *STAT_NAME[4] = {"HP", "ATK", "DEF", "SPD"};

static void show_detail(int index)
{
    const dex_entry_t *e = &dex_entries[index];

    // An lv_image_dsc_t is just a header pointing at pixels; retarget
    // it and the same widget shows any of the 151 sprites in flash.
    sprite_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    sprite_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    sprite_dsc.header.w = DEX_SPRITE_SIZE;
    sprite_dsc.header.h = DEX_SPRITE_SIZE;
    sprite_dsc.header.stride = DEX_SPRITE_SIZE * 2;
    sprite_dsc.data_size = DEX_SPRITE_SIZE * DEX_SPRITE_SIZE * 2;
    sprite_dsc.data = (const uint8_t *)dex_sprites[index];
    lv_image_set_src(detail_sprite, &sprite_dsc);

    lv_label_set_text_fmt(detail_name, "#%03d %s", index + 1, e->name);
    if (e->type2[0])
        lv_label_set_text_fmt(detail_types, "%s / %s", e->type1, e->type2);
    else
        lv_label_set_text(detail_types, e->type1);
    lv_label_set_text_fmt(detail_body, "%d.%d m   %d.%d kg",
                          e->height_dm / 10, e->height_dm % 10,
                          e->weight_hg / 10, e->weight_hg % 10);
    const uint8_t values[4] = {e->hp, e->attack, e->defense, e->speed};
    for (int i = 0; i < 4; i++) {
        lv_bar_set_value(detail_stats[i], values[i], LV_ANIM_OFF);
    }
    lv_screen_load_anim(detail_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0,
                        false);
}

static void row_cb(lv_event_t *ev)
{
    show_detail((int)(intptr_t)lv_event_get_user_data(ev));
}

static void back_cb(lv_event_t *ev)
{
    (void)ev;
    lv_screen_load_anim(list_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 250, 0,
                        false);
}

static void build_list_screen(void)
{
    list_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(list_screen, lv_color_black(), 0);
    lv_obj_set_style_text_font(list_screen, &lv_font_montserrat_28, 0);

    lv_obj_t *list = lv_list_create(list_screen);
    lv_obj_set_size(list, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(list, lv_color_black(), 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_row(list, 2, 0);

    char label[40];
    for (int i = 0; i < DEX_COUNT; i++) {
        snprintf(label, sizeof(label), "#%03d  %s", i + 1,
                 dex_entries[i].name);
        lv_obj_t *btn = lv_list_add_button(list, NULL, label);
        lv_obj_set_style_bg_color(btn, lv_color_black(), 0);
        lv_obj_set_style_text_color(btn, lv_color_white(), 0);
        lv_obj_set_style_pad_ver(btn, 18, 0); // finger-sized rows
        lv_obj_add_event_cb(btn, row_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
    }
}

static void build_detail_screen(void)
{
    detail_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(detail_screen, lv_color_black(), 0);
    lv_obj_set_style_text_font(detail_screen, &lv_font_montserrat_28, 0);

    detail_sprite = lv_image_create(detail_screen);
    lv_obj_align(detail_sprite, LV_ALIGN_TOP_MID, 0, 16);
    lv_image_set_scale(detail_sprite, 512); // 96px art, drawn 192px

    detail_name = lv_label_create(detail_screen);
    lv_obj_set_style_text_color(detail_name, lv_color_hex(ESPTEMBER_ORANGE),
                                0);
    lv_obj_align(detail_name, LV_ALIGN_TOP_MID, 0, 216);

    detail_types = lv_label_create(detail_screen);
    lv_obj_align(detail_types, LV_ALIGN_TOP_MID, 0, 252);

    detail_body = lv_label_create(detail_screen);
    lv_obj_set_style_text_color(detail_body, lv_color_hex(0x888888), 0);
    lv_obj_align(detail_body, LV_ALIGN_TOP_MID, 0, 288);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *tag = lv_label_create(detail_screen);
        lv_label_set_text(tag, STAT_NAME[i]);
        lv_obj_set_style_text_color(tag, lv_color_hex(0x888888), 0);
        lv_obj_align(tag, LV_ALIGN_TOP_LEFT, 16, 330 + i * 26);
        lv_obj_set_style_text_font(tag, &lv_font_montserrat_14, 0);

        detail_stats[i] = lv_bar_create(detail_screen);
        lv_obj_set_size(detail_stats[i], 250, 14);
        lv_obj_align(detail_stats[i], LV_ALIGN_TOP_LEFT, 90, 333 + i * 26);
        lv_bar_set_range(detail_stats[i], 0, 160); // Gen-1 base-stat ceiling
        lv_obj_set_style_bg_color(detail_stats[i],
                                  lv_color_hex(ESPTEMBER_ORANGE),
                                  LV_PART_INDICATOR);
    }

    lv_obj_t *back = lv_button_create(detail_screen);
    lv_obj_set_size(back, 90, 44);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 12, -6);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_label = lv_label_create(back);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
}

void app_main(void)
{
    pmu_init();
    panel_reset_release();
    bsp_display_start();
    bsp_display_backlight_on();

    bsp_display_lock(0);
    build_list_screen();
    build_detail_screen();
    lv_screen_load(list_screen);
    bsp_display_unlock();

    printf("D17_READY entries=%d\n", DEX_COUNT);
}
