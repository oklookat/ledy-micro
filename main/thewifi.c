#include "thewifi.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "ledy-wifi";

static EventGroupHandle_t WIFI_EVENT_GROUP;

static void wifiHandler(void *arg,
                        esp_event_base_t base,
                        int32_t id,
                        void *data)
{
    switch (id)
    {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_CONNECTED:
        xEventGroupSetBits(WIFI_EVENT_GROUP, WIFI_CONNECTED_BIT);
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
    {
        esp_wifi_connect();
        break;
    }
    case IP_EVENT_STA_GOT_IP:
        break;
    default:
        break;
    }
}

static wifi_config_t wifiConfig = {
    .sta = {
        .ssid = LEDY_WIFI_SSID,
        .password = LEDY_WIFI_PASS},
};
static esp_event_handler_instance_t instanceAnyId;

esp_err_t ledyWifiInit(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifiHandler,
                                                        NULL,
                                                        &instanceAnyId));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_err_t retVal = ESP_OK;
    WIFI_EVENT_GROUP = xEventGroupCreate();
    EventBits_t bits = xEventGroupWaitBits(WIFI_EVENT_GROUP,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Conntected");
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGE(TAG, "Connect failed");
        retVal = ESP_FAIL;
    }
    else
    {
        ESP_LOGE(TAG, "Unexpected event");
        retVal = ESP_FAIL;
    }
    /* The event will not be processed after unregister */
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    // vEventGroupDelete(s_wifi_event_group);
    return retVal;
}