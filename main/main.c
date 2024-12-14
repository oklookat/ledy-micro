#include <nvs_flash.h>
#include "theled.h"
#include "themdns.h"
#include "theserver.h"
#include "thewifi.h"

static const char *TAG = "main_boot";

void app_main(void)
{
    ESP_LOGI(TAG, "start");

	// Initialize NVS (for Wi-Fi).
    ESP_LOGI(TAG, "NVS");
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

    // LED.
    ESP_LOGI(TAG, "leds");
    ledyInitLeds();

    // Net.
    ESP_LOGI(TAG, "wi-fi");
    ledyWifiInit();
    ESP_LOGI(TAG, "mdns");
    ledyInitMDNS();

    // WS.
    ESP_LOGI(TAG, "server");
    ledyStartServer();
}
