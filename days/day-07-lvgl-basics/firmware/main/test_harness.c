// Day 07 verification harness.
// Serial protocol over the USB console, in the day-12 tradition:
//   status            -> D07_STATUS screen=<controls|about> presses=N ...
//   tap X Y           -> inject a press/release at screen coords via a
//                        virtual LVGL pointer device, then report status
//   drag X1 Y1 X2 Y2  -> inject a press-move-release gesture
//   touchlog on|off   -> stream raw CST816S coordinates (calibration aid)
//   capture           -> D07_CAPTURE w h RGB565LE + base64 lines + END
// The harness exercises the exact widget tree the lesson builds; only
// the physical touch path is outside its reach.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/usb_serial_jtag.h"
#include "driver/i2c_master.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "mbedtls/base64.h"

extern lv_obj_t *day07_controls_screen;
extern lv_obj_t *day07_about_screen;
extern int day07_press_count;
extern int day07_brightness;
extern bool day07_orange_mode;

// --- Virtual pointer device ---------------------------------------------
// LVGL doesn't care where input comes from. A second pointer indev whose
// read callback replays a scripted gesture is indistinguishable from a
// finger — same hit-testing, same events, same animations.
typedef struct {
    lv_point_t point;
    bool pressed;
} vtouch_sample_t;

#define VTOUCH_MAX 64
static vtouch_sample_t vtouch_queue[VTOUCH_MAX];
static volatile int vtouch_head, vtouch_tail;
static vtouch_sample_t vtouch_last = {{0, 0}, false};

static void vtouch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    if (vtouch_head != vtouch_tail) {
        vtouch_last = vtouch_queue[vtouch_head];
        vtouch_head = (vtouch_head + 1) % VTOUCH_MAX;
    }
    data->point = vtouch_last.point;
    data->state =
        vtouch_last.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->continue_reading = vtouch_head != vtouch_tail;
}

static void vtouch_push(int x, int y, bool pressed)
{
    int next = (vtouch_tail + 1) % VTOUCH_MAX;
    if (next == vtouch_head) return; // full: drop, tests re-check via status
    vtouch_queue[vtouch_tail] =
        (vtouch_sample_t){{.x = x, .y = y}, .pressed = pressed};
    vtouch_tail = next;
}

static void vtouch_wait_idle(void)
{
    while (vtouch_head != vtouch_tail) vTaskDelay(pdMS_TO_TICKS(10));
    vTaskDelay(pdMS_TO_TICKS(120)); // let events + animations land
}

static void inject_tap(int x, int y)
{
    // A few pressed samples so LVGL sees a stable press, then release.
    for (int i = 0; i < 3; i++) vtouch_push(x, y, true);
    vtouch_push(x, y, false);
    vtouch_wait_idle();
}

static void inject_drag(int x1, int y1, int x2, int y2)
{
    const int steps = 12;
    for (int i = 0; i <= steps; i++) {
        vtouch_push(x1 + (x2 - x1) * i / steps, y1 + (y2 - y1) * i / steps,
                    true);
    }
    vtouch_push(x2, y2, false);
    vtouch_wait_idle();
}

// --- Status ---------------------------------------------------------------
static void print_status(void)
{
    const char *screen = "other";
    lv_obj_t *active = lv_screen_active();
    if (active == day07_controls_screen) screen = "controls";
    else if (active == day07_about_screen) screen = "about";
    printf("D07_STATUS screen=%s presses=%d brightness=%d orange=%d heap=%u\n",
           screen, day07_press_count, day07_brightness,
           (int)day07_orange_mode, (unsigned)esp_get_free_heap_size());
}

// --- Screen capture -------------------------------------------------------
static void capture(void)
{
    bsp_display_lock(0);
    lv_draw_buf_t *buf =
        lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_RGB565);
    bsp_display_unlock();
    if (!buf) {
        printf("D07_CAPTURE_FAILED\n");
        return;
    }
    int w = buf->header.w, h = buf->header.h, stride = buf->header.stride;
    printf("D07_CAPTURE %d %d RGB565LE\n", w, h);
    // Base64, one row per line — survives console CRLF translation.
    unsigned char line[1024];
    for (int y = 0; y < h; y++) {
        size_t out = 0;
        mbedtls_base64_encode(line, sizeof(line), &out,
                              buf->data + y * stride, w * 2);
        fwrite(line, 1, out, stdout);
        fputc('\n', stdout);
        // Yield so the idle task feeds the watchdog during the ~440 KB dump.
        if ((y & 31) == 31) {
            fflush(stdout);
            vTaskDelay(1);
        }
    }
    printf("D07_CAPTURE_END\n");
    fflush(stdout);
    lv_draw_buf_destroy(buf);
}

// --- Physical-touch stream (calibration aid) --------------------------------
// Reads the BSP's own LVGL indev — the exact points LVGL hit-tests
// with. (A parallel raw I2C poll races the BSP driver for the chip's
// latched registers and corrupts both readers — learned the hard way.)
static volatile bool touchlog_enabled = false;

static lv_obj_t *touch_dot;
static lv_obj_t *cal_target;

// Center-anchored stretch correction, percent. 100 = off. If the
// controller maps a taller glass onto the panel, reported coords are
// stretched about the center; divide the deviation to undo it.
static int cal_pct_x = 100, cal_pct_y = 100;

static lv_point_t cal_apply(lv_point_t p)
{
    p.x = 184 + (int)(p.x - 184) * 100 / cal_pct_x;
    p.y = 224 + (int)(p.y - 224) * 100 / cal_pct_y;
    return p;
}

static void touchlog_task(void *arg)
{
    (void)arg;
    while (1) {
        if (touchlog_enabled) {
            lv_indev_t *indev = bsp_display_get_input_dev();
            if (indev &&
                lv_indev_get_state(indev) == LV_INDEV_STATE_PRESSED) {
                lv_point_t p;
                lv_indev_get_point(indev, &p);
                printf("D07_RAWTP x=%d y=%d\n", (int)p.x, (int)p.y);
                p = cal_apply(p); // dot shows the corrected position
                bsp_display_lock(0);
                if (!touch_dot) {
                    touch_dot = lv_obj_create(lv_layer_top());
                    lv_obj_set_size(touch_dot, 16, 16);
                    lv_obj_set_style_radius(touch_dot, LV_RADIUS_CIRCLE, 0);
                    lv_obj_set_style_bg_color(touch_dot,
                                              lv_color_hex(0x00ff00), 0);
                    lv_obj_set_style_border_width(touch_dot, 0, 0);
                    lv_obj_remove_flag(touch_dot, LV_OBJ_FLAG_CLICKABLE);
                }
                lv_obj_set_pos(touch_dot, p.x - 8, p.y - 8);
                lv_obj_remove_flag(touch_dot, LV_OBJ_FLAG_HIDDEN);
                bsp_display_unlock();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// --- Command loop -----------------------------------------------------------
static void harness_task(void *arg)
{
    (void)arg;
    // Blocking line reads from the USB-Serial/JTAG console.
    usb_serial_jtag_driver_config_t cfg =
        USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    usb_serial_jtag_driver_install(&cfg);
    usb_serial_jtag_vfs_use_driver();

    char line[128];
    while (fgets(line, sizeof(line), stdin)) {
        char *nl = strpbrk(line, "\r\n");
        if (nl) *nl = 0;
        int x, y, x2, y2;
        if (!strcmp(line, "status")) {
            print_status();
        } else if (sscanf(line, "tap %d %d", &x, &y) == 2) {
            inject_tap(x, y);
            print_status();
        } else if (sscanf(line, "drag %d %d %d %d", &x, &y, &x2, &y2) == 4) {
            inject_drag(x, y, x2, y2);
            print_status();
        } else if (!strcmp(line, "touchlog on")) {
            touchlog_enabled = true;
            printf("D07_TOUCHLOG on\n");
        } else if (!strcmp(line, "touchlog off")) {
            touchlog_enabled = false;
            printf("D07_TOUCHLOG off\n");
        } else if (sscanf(line, "target %d %d", &x, &y) == 2) {
            bsp_display_lock(0);
            if (!cal_target) {
                cal_target = lv_obj_create(lv_layer_top());
                lv_obj_set_size(cal_target, 24, 24);
                lv_obj_set_style_radius(cal_target, LV_RADIUS_CIRCLE, 0);
                lv_obj_set_style_bg_color(cal_target, lv_color_hex(0xff0000),
                                          0);
                lv_obj_set_style_border_width(cal_target, 0, 0);
                lv_obj_remove_flag(cal_target, LV_OBJ_FLAG_CLICKABLE);
            }
            lv_obj_set_pos(cal_target, x - 12, y - 12);
            lv_obj_remove_flag(cal_target, LV_OBJ_FLAG_HIDDEN);
            bsp_display_unlock();
            printf("D07_TARGET %d %d\n", x, y);
        } else if (sscanf(line, "cal %d %d", &x, &y) == 2) {
            cal_pct_x = x > 0 ? x : 100;
            cal_pct_y = y > 0 ? y : 100;
            printf("D07_CAL x=%d y=%d\n", cal_pct_x, cal_pct_y);
        } else if (!strcmp(line, "target off")) {
            if (cal_target) {
                bsp_display_lock(0);
                lv_obj_add_flag(cal_target, LV_OBJ_FLAG_HIDDEN);
                bsp_display_unlock();
            }
            printf("D07_TARGET off\n");
        } else if (!strcmp(line, "capture")) {
            capture();
        } else if (line[0]) {
            printf("D07_REJECTED %s\n", line);
        }
    }
    vTaskDelete(NULL);
}

void day07_test_harness_init(void)
{
    bsp_display_lock(0);
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, vtouch_read_cb);
    bsp_display_unlock();

    xTaskCreate(harness_task, "d07_harness", 6144, NULL, 5, NULL);
    xTaskCreate(touchlog_task, "d07_touchlog", 4096, NULL, 4, NULL);
}
