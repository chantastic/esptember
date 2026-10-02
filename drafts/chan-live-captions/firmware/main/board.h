// Waveshare AMOLED 1.8 V2 bring-up, from ESPtember days 27/32. Not portable to other boards.
#include "driver/i2c_master.h"
#include "esp_io_expander_tca9554.h"
static i2c_master_dev_handle_t pmu;
static uint8_t pmu_read(uint8_t reg) { uint8_t val=0; ESP_ERROR_CHECK(i2c_master_transmit_receive(pmu,&reg,1,&val,1,100)); return val; }
static void pmu_write(uint8_t reg,uint8_t val) { uint8_t data[]={reg,val}; ESP_ERROR_CHECK(i2c_master_transmit(pmu,data,2,100)); }
static void board_init(void) {
    ESP_ERROR_CHECK(bsp_i2c_init());
    i2c_device_config_t cfg={.device_address=0x34,.scl_speed_hz=100000};
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bsp_i2c_get_handle(),&cfg,&pmu));
    pmu_write(0x16,(pmu_read(0x16)&0xF8)|0x02);
    pmu_write(0x62,(pmu_read(0x62)&0xE0)|0x09);
    esp_io_expander_handle_t expander=NULL;
    ESP_ERROR_CHECK(esp_io_expander_new_i2c_tca9554(bsp_i2c_get_handle(),ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000,&expander));
    const uint32_t pins=IO_EXPANDER_PIN_NUM_0|IO_EXPANDER_PIN_NUM_1|IO_EXPANDER_PIN_NUM_2;
    ESP_ERROR_CHECK(esp_io_expander_set_dir(expander,pins,IO_EXPANDER_OUTPUT));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander,pins,1)); vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander,pins,0)); vTaskDelay(pdMS_TO_TICKS(300));
    ESP_ERROR_CHECK(esp_io_expander_set_level(expander,pins,1));
}
