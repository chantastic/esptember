// ESPtember Day 09 — Sound Effects Board
// Day 08 made the screen an interface. Today the board gets a voice:
// six synthesized effects on touch pads, three per page, played through
// the ES8311 codec and the onboard speaker. No audio files — every
// sound is generated from math at press time. Board bring-up
// (pmu_init, panel_reset_release) carries over from day 01.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c_master.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "esp_codec_dev.h"
#include "sounds.h"

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

// --- Audio ----------------------------------------------------------------
// One playback task owns the codec. Pads don't play sounds — they queue
// a sound id and return immediately, so the UI never blocks on audio.
// The queue length is 1 and sends don't wait: mashing pads restarts
// nothing and stacks nothing; the current effect finishes, the latest
// request plays next.

// Shared with the verification harness.
int day08_page = 0;
int day08_last_sound = -1;
int day08_play_count = 0;
volatile bool day08_playing = false;
void day08_test_harness_init(void);

static esp_codec_dev_handle_t speaker;
static QueueHandle_t sound_queue;

static void audio_task(void *arg)
{
    (void)arg;
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = SOUND_SAMPLE_RATE,
        .channel = 1,
        .bits_per_sample = 16,
    };
    esp_codec_dev_open(speaker, &fs);
    esp_codec_dev_set_out_vol(speaker, 80);
    int id;
    while (xQueueReceive(sound_queue, &id, portMAX_DELAY)) {
        int samples = 0;
        const int16_t *pcm = sound_render(id, &samples);
        if (!pcm) continue;
        day08_playing = true;
        esp_codec_dev_write(speaker, (void *)pcm, samples * sizeof(int16_t));
        day08_playing = false;
    }
}

void day08_play(int id)
{
    day08_last_sound = id;
    day08_play_count++;
    xQueueSend(sound_queue, &id, 0);
}

// --- UI ---------------------------------------------------------------------
#define ESPTEMBER_ORANGE 0xff5b04
#define PAGE_COUNT 2
#define PADS_PER_PAGE 3

static lv_obj_t *pages[PAGE_COUNT];
static lv_obj_t *page_label;

static void pad_cb(lv_event_t *e)
{
    day08_play((int)(intptr_t)lv_event_get_user_data(e));
}

static void show_page(int page)
{
    day08_page = page;
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (i == page) lv_obj_remove_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text_fmt(page_label, "%d / %d", page + 1, PAGE_COUNT);
}

static void page_prev_cb(lv_event_t *e)
{
    (void)e;
    show_page((day08_page + PAGE_COUNT - 1) % PAGE_COUNT);
}

static void page_next_cb(lv_event_t *e)
{
    (void)e;
    show_page((day08_page + 1) % PAGE_COUNT);
}

static void build_ui(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_text_font(screen, &lv_font_montserrat_28, 0);

    // Pad pages: one flex column each, three fat pads per page.
    for (int p = 0; p < PAGE_COUNT; p++) {
        lv_obj_t *col = lv_obj_create(screen);
        lv_obj_set_size(col, lv_pct(94), 360);
        lv_obj_align(col, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_SPACE_EVENLY,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        for (int i = 0; i < PADS_PER_PAGE; i++) {
            int id = p * PADS_PER_PAGE + i;
            lv_obj_t *pad = lv_button_create(col);
            lv_obj_set_size(pad, lv_pct(100), 92);
            lv_obj_set_style_bg_color(pad, lv_color_hex(ESPTEMBER_ORANGE), 0);
            lv_obj_add_event_cb(pad, pad_cb, LV_EVENT_CLICKED,
                                (void *)(intptr_t)id);
            lv_obj_t *label = lv_label_create(pad);
            lv_label_set_text(label, sounds[id].name);
            lv_obj_center(label);
        }
        pages[p] = col;
    }

    // Pager row: previous / indicator / next.
    lv_obj_t *row = lv_obj_create(screen);
    lv_obj_set_size(row, lv_pct(94), 72);
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *prev = lv_button_create(row);
    lv_obj_set_size(prev, 90, 64);
    lv_obj_add_event_cb(prev, page_prev_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *prev_label = lv_label_create(prev);
    lv_label_set_text(prev_label, LV_SYMBOL_LEFT);
    lv_obj_center(prev_label);

    page_label = lv_label_create(row);

    lv_obj_t *next = lv_button_create(row);
    lv_obj_set_size(next, 90, 64);
    lv_obj_add_event_cb(next, page_next_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next_label = lv_label_create(next);
    lv_label_set_text(next_label, LV_SYMBOL_RIGHT);
    lv_obj_center(next_label);

    show_page(0);
}

void app_main(void)
{
    pmu_init();
    panel_reset_release();

    // Audio before display: the codec shares the I2C bus already up.
    bsp_audio_init(NULL);
    speaker = bsp_audio_codec_speaker_init();
    sound_queue = xQueueCreate(1, sizeof(int));
    xTaskCreate(audio_task, "audio", 4096, NULL, 6, NULL);

    bsp_display_start();
    bsp_display_backlight_on();

    bsp_display_lock(0);
    build_ui();
    bsp_display_unlock();

    day08_test_harness_init();
}
