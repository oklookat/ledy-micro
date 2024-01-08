#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>
#include <vector>

#include <WebSocketsServer.h>
#include <ESPmDNS.h>

// You need to create "env.h" file
// with contents like this:
// #pragma once
// const char *SSID = "YOUR-WI-FI-SSID";
// const char *PASSWORD = "YOUR-WI-FI-PASSWORD";
#include "env.h"

// mDNS.
const char *M_DNS_HOSTNAME = "ledy-server";
const char *M_DNS_SERVICE = "_ledy";
const char *M_DNS_PROTO = "_tcp";
constexpr unsigned short M_DNS_PORT = 81;

// Websockets.
WebSocketsServer webSocket = WebSocketsServer(M_DNS_PORT);

// LED.
constexpr unsigned short NUM_LEDS = 300;
constexpr unsigned char PIN_1 = 32;
constexpr unsigned char PIN_2 = 33;
constexpr unsigned char PIN_3 = 25;
bool isProcessed = true;
CRGB THE_LEDS[NUM_LEDS];
// CRGB LEDS[3][NUM_LEDS];
std::vector<uint8_t> LED_DATA;
TaskHandle_t LED_TASK = nullptr;
QueueHandle_t LED_QUEUE = nullptr;

// Put function declarations here:
void clearLeds();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void netConnect();
void onWiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void setLeds(void *data);
//

void setup()
{
  Serial.begin(115200);

  for (uint8_t t = 4; t > 0; t--)
  {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  // LED to second core.
  LED_QUEUE = xQueueCreate(1, 0);
  if (LED_QUEUE == NULL)
  {
    while (true)
    {
      Serial.printf("Failed to create LED_QUEUE");
      delay(5000);
    }
  }
  xTaskCreatePinnedToCore(
      setLeds,
      "Leds",
      8092,
      NULL,
      1,
      &LED_TASK,
      1);

  // Wi-Fi.
  WiFi.mode(WIFI_STA);
  netConnect();
  WiFi.onEvent(onWiFiDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // LED.
  FastLED.addLeds<WS2812B, PIN_1, GRB>(THE_LEDS, NUM_LEDS);
  FastLED.setMaxRefreshRate(0, false);
  FastLED.setCorrection(Typical8mmPixel);
  FastLED.setTemperature(HighNoonSun);
  clearLeds();

  return;
}

void loop()
{
  webSocket.loop();
  // get led task stack size:
  // auto t2 = uxTaskGetStackHighWaterMark(ledTask);
  // Serial.printf("T2: %d\n", t2);
  return;
}

void clearLeds()
{
  FastLED.clear();
  FastLED.show();
  return;
}

// WebSocket server.
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
  {
    break;
  }
  case WStype_CONNECTED:
  {
    break;
  }
  case WStype_TEXT:
  {
    // webSocket.sendTXT(num, payload, length);
    // String pStr = (char*)payload;
    // webSocket.sendTXT(num, "command not found");
    break;
  }

  case WStype_BIN:
  {
    if (!isProcessed)
    {
      break;
    }
    LED_DATA = std::vector<uint8_t>(payload, payload + length);
    xQueueSend(LED_QUEUE, NULL, 0);
    break;
  }
  case WStype_ERROR:
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    break;
  }
  return;
}

void netConnect()
{
  // Wi-Fi.
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wi-Fi: connecting...");
    delay(500);
  }
  Serial.println("Wi-Fi: connected. IP: " + WiFi.localIP().toString());

  // mDNS.
  if (!MDNS.begin(M_DNS_HOSTNAME))
  {
    Serial.println("mDNS: error setting up MDNS responder");
    while (1)
    {
      delay(1000);
    }
  }
  MDNS.addService(M_DNS_SERVICE, M_DNS_PROTO, M_DNS_PORT);
  Serial.println("mDNS: responder started");

  // WS.
  webSocket.close();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  return;
}

// Try recconect to Wi-Fi if disconnected.
void onWiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println(info.wifi_sta_disconnected.reason);
  netConnect();
  return;
}

// Set LED's from websocket data.
void setLeds(void *data)
{
  while (1)
  {
    if (xQueueReceive(LED_QUEUE, NULL, pdMS_TO_TICKS(60000)) != pdTRUE)
    {
      continue;
    }

    isProcessed = false;
    auto ledDataSize = LED_DATA.size();
    auto ledCount = LED_DATA.size() / 3;
    unsigned int byteIdx = 0;

    for (size_t ledIdx = 0; ledIdx < ledCount; ledIdx++)
    {
      if (ledIdx >= NUM_LEDS || byteIdx + 2 >= ledDataSize)
      {
        break;
      }
      THE_LEDS[ledIdx].setRGB(LED_DATA[byteIdx], LED_DATA[byteIdx + 1], LED_DATA[byteIdx + 2]);
      byteIdx += 3;
    }

    FastLED.show();
    isProcessed = true;
  }
  return;
}
