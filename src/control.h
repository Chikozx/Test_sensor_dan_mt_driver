#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "string.h"
#include "main.h"

#define CONTROL_TAG "Control"

void receive_control(uint8_t* data , uint16_t len);