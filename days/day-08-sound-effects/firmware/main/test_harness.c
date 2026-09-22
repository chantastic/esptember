// Day 08 verification harness.
// Serial protocol over the USB console, in the day-12 tradition:
//   status            -> D08_STATUS screen=<controls|about> presses=N ...
//   tap X Y           -> inject a press/release at screen coords via a
//                        virtual LVGL pointer device, then report status
//   drag X1 Y1 X2 Y2  -> inject a press-move-release gesture
//   touchlog on|off   -> stream raw CST816S coordinates (calibration aid)
//   capture           -> D08_CAPTURE w h RGB565LE + base64 lines + END
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

extern int day08_page;
extern int day08_last_sound;
extern int day08_play_count;
extern volatile bool day08_playing;

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
    printf("D08_STATUS page=%d last=%d plays=%d playing=%d heap=%u\n",
           day08_page, day08_last_sound, day08_play_count,
           (int)day08_playing, (unsigned)esp_get_free_heap_size());
}

// --- Screen capture -------------------------------------------------------
static void capture(void)
{
    bsp_display_lock(0);
    lv_draw_buf_t *buf =
        lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_RGB565);
    bsp_display_unlock();
    if (!buf) {
        printf("D08_CAPTURE_FAILED\n");
        return;
    }
    int w = buf->header.w, h = buf->header.h, stride = buf->header.stride;
    printf("D08_CAPTURE %d %d RGB565LE\n", w, h);
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
    printf("D08_CAPTURE_END\n");
    fflush(stdout);
    lv_draw_buf_destroy(buf);
}

// --- Raw CST816S stream (physical-touch calibration aid) -------------------
static volatile bool touchlog_enabled = false;
static i2c_master_dev_handle_t tp_dev;

static void touchlog_task(void *arg)
{
    (void)arg;
    i2c_device_config_t cfg = {
        .device_address = 0x15,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(bsp_i2c_get_handle(), &cfg, &tp_dev);
    while (1) {
        if (touchlog_enabled) {
            uint8_t reg = 0x01, buf[6] = {0};
            if (i2c_master_transmit_receive(tp_dev, &reg, 1, buf, 6, 100) ==
                    ESP_OK &&
                buf[1]) {
                int x = ((buf[2] & 0x0F) << 8) | buf[3];
                int y = ((buf[4] & 0x0F) << 8) | buf[5];
                printf("D08_RAWTP x=%d y=%d\n", x, y);
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
            printf("D08_TOUCHLOG on\n");
        } else if (!strcmp(line, "touchlog off")) {
            touchlog_enabled = false;
            printf("D08_TOUCHLOG off\n");
        } else if (!strcmp(line, "capture")) {
            capture();
        } else if (line[0]) {
            printf("D08_REJECTED %s\n", line);
        }
    }
    vTaskDelete(NULL);
}

void day08_test_harness_init(void)
{
    bsp_display_lock(0);
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, vtouch_read_cb);
    bsp_display_unlock();

    xTaskCreate(harness_task, "d08_harness", 6144, NULL, 5, NULL);
    xTaskCreate(touchlog_task, "d08_touchlog", 4096, NULL, 4, NULL);
}
