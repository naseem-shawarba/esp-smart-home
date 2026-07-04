// ESP-NOW orchestrator / gateway.
// Receives sensor messages (see shared/espnow_protocol) and acts on them

#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "espnow_protocol.h"
#include "credentials.h"
#include "device_config.h"
#include "wifi_link.h"
#include "web_portal.h"
#include "weather_config.h"

using namespace espnow_protocol;

#ifndef BUILTIN_LED_PIN
#define BUILTIN_LED_PIN -1
#endif

#ifndef PORTAL_PIN
#define PORTAL_PIN -1
#endif

static void logHeader(const uint8_t *mac, const Header &h)
{
  Serial.printf("[node %02X:%02X:%02X:%02X:%02X:%02X id=%u seq=%u batt=%u] ",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                h.nodeId, (unsigned)h.seq, h.batteryPct);
}

// arduino-esp32 2.0.17 receive-callback signature.
static void onReceive(const uint8_t *mac, const uint8_t *data, int len)
{
  if (len < (int)sizeof(Header))
  {
    return;
  }

  Header h;
  memcpy(&h, data, sizeof(h));
  if (h.version != PROTOCOL_VERSION)
  {
    Serial.printf("Ignoring message with protocol version %u\n", h.version);
    return;
  }

  switch ((DeviceType)h.deviceType)
  {
  case DeviceType::AlarmNode:
  {
    // TO-DO
    break;
  }

  case DeviceType::WeatherNode:
  {
    // TO-DO
    break;
  }

  default:
    logHeader(mac, h);
    Serial.printf("UNKNOWN deviceType=%u len=%d\n", h.deviceType, len);
    break;
  }
}


static bool portalButtonClicked()
{
#if PORTAL_PIN >= 0
  return digitalRead(PORTAL_PIN) == LOW;
#else
  return false;
#endif
  
}

static void tick()
{
  // Blink blue while the portal is open (stealth-aware via rgb_led).
  if (millis() % 1500 > 750)
  {
    digitalWrite(BUILTIN_LED_PIN, HIGH);
  }
  else
  {
    digitalWrite(BUILTIN_LED_PIN, LOW);
  }
}

static void openPortal()
{
  web_portal::Options opts;
  opts.deviceName = device_config::name();
  opts.durationMs = device_config::portalWindowMs();
  opts.showMeshFields = false;
  opts.extraMenuLabel = "Settings";
  opts.extraMenuHref = "/settings";
  opts.registerExtraRoutes = weather_config::registerRoutes;
  opts.shouldExit = portalButtonClicked;
  opts.onTick = tick;

  web_portal::run(opts);
}

static void startEspNow()
{
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(onReceive);
  Serial.println("Orchestrator ready — listening for ESP-NOW messages");
}

void setup()
{
  Serial.begin(115200);
  delay(500);

#if PORTAL_PIN >= 0
  pinMode(PORTAL_PIN, INPUT_PULLUP);
#endif

#if BUILTIN_LED_PIN
  pinMode(BUILTIN_LED_PIN, OUTPUT);
#endif

  if (!credentials::isConfigured())
  {
    Serial.println("Not configured; opening setup portal");
    openPortal();
  }

  wifi_link::connect();

  Serial.print("Orchestrator MAC (enter this in each alarm node's portal): ");
  Serial.println(WiFi.macAddress());
  Serial.printf("ESP-NOW channel: %d\n", WiFi.channel());

  startEspNow();
}

void loop()
{
  if (portalButtonClicked())
  {
    openPortal();
    ESP.restart();
  }
  // TO-DO Take actions e.g. send post requests, send telegram messages or notify another ESP32.

  delay(20);
}
