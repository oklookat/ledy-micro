#include "theled.h"

static led_strip_handle_t LED_STRIP;
static TaskHandle_t LED_TASK = NULL;
static QueueHandle_t LED_QUEUE = NULL;

typedef struct LedQueueData
{
    uint8_t *payload;
    size_t length;
} LedQueueData;

bool ledySetLeds(uint8_t *payload, size_t length)
{
    if (length < 3 || payload == NULL)
    {
        return true;
    }
    LedQueueData *data = malloc(sizeof(LedQueueData));
    data->length = length;
    data->payload = payload;
    BaseType_t xResult = xQueueSend(LED_QUEUE, (void *)&data, 0);
    if (xResult != pdTRUE)
    {
        free(data);
        return true;
    }
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
        struct LedQueueData *data;
        BaseType_t xResult = xQueueReceive(LED_QUEUE, (void *)&data, portMAX_DELAY);
        if (xResult != pdTRUE)
        {
            continue;
        }

        // 3 = idx0[1 BYTE COMMAND] idx1-2[2 BYTE LED DATA LEN] idx3[? BYTES LED DATA G R B].

        unsigned int ledsLen = bytesToUint16(data->payload[2], data->payload[1]);
        size_t stripI = 0;
        for (size_t i = 3; i - 3 < ledsLen; i += 3)
        {
            led_strip_set_pixel(LED_STRIP, stripI,
                                data->payload[i + 1],
                                data->payload[i],
                                data->payload[i + 2]);
            stripI++;
        }

        free(data->payload);
        free(data);
        led_strip_refresh(LED_STRIP);
    }
    return;
}

static led_strip_config_t STRIP_CONFIG = {
    .strip_gpio_num = LEDY_PIN_1,
    .max_leds = 900,
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
    LED_QUEUE = xQueueCreate(1, sizeof(LedQueueData));
    xTaskCreatePinnedToCore(
        ledySetLedsTask,
        "LEDYLEDS",
        8092,
        NULL,
        configMAX_PRIORITIES - 1,
        &LED_TASK,
        1);
    gpio_reset_pin(LEDY_PIN_1);
    gpio_set_direction(LEDY_PIN_1, GPIO_MODE_OUTPUT);
    return led_strip_new_spi_device(&STRIP_CONFIG, &SPI_CONFIG, &LED_STRIP);
}