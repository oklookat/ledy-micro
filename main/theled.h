#ifndef LEDY_LED
#define LEDY_LED

#include <esp_log.h>
#include <driver/gpio.h>
#include <semaphore.h>
#include "led_strip.h"

#define LEDY_PIN_1 32

esp_err_t ledyInitLeds(void);
bool ledySetLeds(uint8_t *payload, size_t length);

#endif