// QSPI display path with a small, preallocated internal DMA buffer.
// The BSP default uses the RGB helper and a non-DMA buffer; SPI then tries to
// allocate private DMA copies during every flush, which fails under TLS/audio load.
#include "esp_lcd_touch.h"
#include "esp_lvgl_port.h"
#include "esp_lvgl_port_touch.h"
#include "bsp/display.h"
#include "bsp/touch.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
static void round_area(lv_event_t *event) {
    lv_area_t *area=lv_event_get_param(event);
    area->x1&=~1; area->y1&=~1; area->x2|=1; area->y2|=1;
}
static void display_start(void) {
    lvgl_port_cfg_t port=ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&port));
    esp_lcd_panel_handle_t panel; esp_lcd_panel_io_handle_t io;
    bsp_display_config_t config={0}; ESP_ERROR_CHECK(bsp_display_new(&config,&panel,&io));
    lvgl_port_display_cfg_t lcd={
        .io_handle=io,.panel_handle=panel,.buffer_size=368*32,.double_buffer=false,
        .hres=368,.vres=448,.color_format=LV_COLOR_FORMAT_RGB565,
        .flags={.buff_dma=true,.buff_spiram=false,.swap_bytes=true},
    };
    lv_display_t *display=lvgl_port_add_disp(&lcd); assert(display);
    lv_display_add_event_cb(display,round_area,LV_EVENT_INVALIDATE_AREA,NULL);
    esp_lcd_touch_handle_t touch;
    ESP_ERROR_CHECK(bsp_touch_new(NULL,&touch));
    lvgl_port_touch_cfg_t input={.disp=display,.handle=touch};
    assert(lvgl_port_add_touch(&input));
    ESP_ERROR_CHECK(bsp_display_brightness_init());
}
