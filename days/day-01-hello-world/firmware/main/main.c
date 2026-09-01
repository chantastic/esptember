// ESPtember Day 01 — Hello World
// The smallest possible ESP32-S3 program: print over USB, forever.
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    while (true) {
        printf("Hello, ESPtember!\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
