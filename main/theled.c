#include "theled.h"

static const char *TAG = "ledy-led";

static led_strip_handle_t LED_STRIP;
static TaskHandle_t LED_TASK = NULL;

typedef struct LedData
{
    uint8_t *payload;
    size_t length;
} LedData;
static LedData *LED_DATA = NULL;

#define MAX_LEDS 300
// Depends on MAX_LEDS.
// 300 LEDS = 6600.
#define LED_TASK_STACK_SIZE 6700

bool ledySetLeds(uint8_t *payload, size_t length)
{
    if (length < 3 || payload == NULL || LED_DATA != NULL)
    {
        return true;
    }

    //ESP_LOGI(TAG, "LED TASK STACK: %d", uxTaskGetStackHighWaterMark(LED_TASK));

    LED_DATA = malloc(sizeof(LedData));
    LED_DATA->length = length;
    LED_DATA->payload = payload;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveIndexedFromISR(LED_TASK, 0, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return false;
}

static inline uint16_t bytesToUint16(uint8_t byte1, uint8_t byte2)
{
    return (byte1 << 8) | byte2;
}

static void ledySetLedsTask(void *pvParameters)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (LED_DATA == NULL)
        {
            continue;
        }

        // 3 = idx0[1 BYTE COMMAND] idx1-2[2 BYTE LED DATA LEN] idx3[? BYTES LED DATA G R B].

        unsigned int ledsLen = bytesToUint16(LED_DATA->payload[2], LED_DATA->payload[1]);
        size_t stripI = 0;
        for (size_t i = 3; i - 3 < ledsLen; i += 3)
        {
            led_strip_set_pixel(LED_STRIP, stripI,
                                LED_DATA->payload[i + 1],
                                LED_DATA->payload[i],
                                LED_DATA->payload[i + 2]);
            stripI++;
        }

        free(LED_DATA->payload);
        free(LED_DATA);
        LED_DATA = NULL;
        ESP_ERROR_CHECK(led_strip_refresh(LED_STRIP));
    }
    return;
}

static led_strip_config_t STRIP_CONFIG = {
    .strip_gpio_num = LEDY_PIN_1,
    .max_leds = MAX_LEDS,
    .led_pixel_format = LED_PIXEL_FORMAT_GRB,
    .led_model = LED_MODEL_WS2812,
    .flags.invert_out = false,
};

static led_strip_spi_config_t SPI_CONFIG = {
    .clk_src = SPI_CLK_SRC_DEFAULT,
    .flags.with_dma = true,
    .spi_bus = SPI2_HOST,
};

esp_err_t ledyInitLeds(void)
{
    xTaskCreatePinnedToCore(
        ledySetLedsTask,
        "LEDYLEDS",
        LED_TASK_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 1,
        &LED_TASK,
        1);
    gpio_reset_pin(LEDY_PIN_1);
    gpio_set_direction(LEDY_PIN_1, GPIO_MODE_OUTPUT);
    return led_strip_new_spi_device(&STRIP_CONFIG, &SPI_CONFIG, &LED_STRIP);
}