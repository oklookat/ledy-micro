#include <Arduino.h>
#include <vector>
#include <array>

#include <WiFi.h>
#include <FastLED.h>

#include <WebSocketsServer.h>
#include <ESPmDNS.h>

#include "util.h"

// If you want specify ssid and password:
// Uncomment wifi.begin() line in netConnect() and
// you need to create "env.h" file
// with contents like this:
// #pragma once
// const char *SSID = "YOUR-WI-FI-SSID";
// const char *PASSWORD = "YOUR-WI-FI-PASSWORD";
// #include "env.h"

// mDNS.
const char *M_DNS_HOSTNAME = "ledy-server";
const char *M_DNS_SERVICE = "_ledy";
const char *M_DNS_PROTO = "_tcp";
constexpr unsigned short M_DNS_PORT = 81;

// Websockets.
WebSocketsServer webSocket = WebSocketsServer(M_DNS_PORT);

// LED.
constexpr unsigned short NUM_LEDS = 900;
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
void processCommand(uint8_t *payload, size_t length);
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

  // Wi-Fi.
  netConnect();
  WiFi.onEvent(onWiFiDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // LED.
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
  //  get led task stack size:
  //  auto t2 = uxTaskGetStackHighWaterMark(ledTask);
  //  Serial.printf("T2: %d\n", t2);
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
  case WStype_BIN:
  {
    if (!isProcessed || length < 1 || payload == nullptr)
    {
      break;
    }
    processCommand(payload, length);
    webSocket.sendTXT(num, "command executed");
    break;
  }
  }
  return;
}

void processCommand(uint8_t *payload, size_t length)
{
  if (payload == nullptr || length == 0) // Проверка на нулевой указатель и нулевую длину
    return;

  constexpr unsigned char COMMAND_SET_COLORS = 0;
  constexpr unsigned char COMMAND_SET_CORRECTION = 1;
  constexpr unsigned char COMMAND_SET_TEMPERATURE = 2;
  constexpr unsigned char COMMAND_SET_WIFI = 3;
  switch (payload[0])
  {
  case COMMAND_SET_COLORS:
  {
    auto ledsLen = bytesToUint16(payload[2], payload[1]);
    LED_DATA = std::vector<uint8_t>(payload + 3, payload + 3 + ledsLen);
    xQueueSend(LED_QUEUE, NULL, 0);
    break;
  }
  case COMMAND_SET_CORRECTION:
  {
    auto corr = bytesToUint32(payload[4], payload[3], payload[2], payload[1]);
    FastLED.setCorrection(corr);
    break;
  }
  case COMMAND_SET_TEMPERATURE:
  {
    auto temp = bytesToUint32(payload[4], payload[3], payload[2], payload[1]);
    FastLED.setTemperature(temp);
    break;
  }
  case COMMAND_SET_WIFI:
  {
    // [1 BYTE HEADER][1-32 BYTE SSID][1-32 BYTE PASSWORD].
    // IF SSID OR PASSWORD LENGTH < 32 IT MUST BE FILLED WITH
    // NULL TERM. (0).
    if (length != 65)
      return;

    std::array<uint8_t, 32> ssid;
    copyAndCheckTerminator(payload + 1, ssid);

    std::array<uint8_t, 32> password;
    copyAndCheckTerminator(payload + 1 + 32, password);

    write32EEPROM(ssid, 0);
    write32EEPROM(password, 32);

    break;
  }
  default:
  {
    break;
  }
  }
}

void netConnect()
{
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ledy", "setpassplz");

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

  // Wi-Fi.

  // Specify ssid and password:
  // WiFi.begin(SSID, PASSWORD);

  // SSID + password.
  // Comment this block if you want specify ssid and password.
  // Btw this block doesnt work because mDNS doesnt see ESP32 when
  // i connect to ESP32 WiFi AP, or something like this. Idk how to fix it.
  // It was assumed that it would be possible to set
  // the ssid and password from wi fi through
  // an access point, connecting to it, for example, from a smartphone.
  auto ssid = getSSID();
  auto password = getPassword();
  Serial.printf("SSID: %s\n", ssid.c_str());
  Serial.printf("Password: %s\n", password.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wi-Fi: connecting...");
    delay(500);
  }
  WiFi.softAPdisconnect(false);
  WiFi.mode(WIFI_STA);
  Serial.println("Wi-Fi: connected. IP: " + WiFi.localIP().toString());

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
      THE_LEDS[ledIdx].setRGB(LED_DATA[byteIdx + 2], LED_DATA[byteIdx + 1], LED_DATA[byteIdx]);
      byteIdx += 3;
    }

    FastLED.show();
    isProcessed = true;
  }
  return;
}