#ifndef LEDY_WIFI
#define LEDY_WIFI

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <freertos/event_groups.h>


/* WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define LEDY_WIFI_SSID "mywifissid"
*/

#define LEDY_WIFI_SSID      CONFIG_LEDY_WIFI_SSID
#define LEDY_WIFI_PASS      CONFIG_LEDY_WIFI_PASSWORD

esp_err_t ledyWifiInit(void);

#endif