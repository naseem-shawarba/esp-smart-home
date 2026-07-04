// ESP-NOW orchestrator / gateway.
// Receives sensor messages (see shared/espnow_protocol) and acts on them:
// - weather readings are POSTed as JSON to a server endpoint
// - alarm events are logged (Telegram notifications / siren activation still TODO)

#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
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

static QueueHandle_t weatherQueue = nullptr;

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
    if (len < (int)sizeof(GuardMsg))
    {
      return;
    }
    GuardMsg m;
    memcpy(&m, data, sizeof(m));
    logHeader(mac, h);
    Serial.printf("ALARM event=%u armed=%u door=%u triggers=%u\n",
                  h.event, m.armed, m.doorOpen, m.triggerCount);
    // TODO: take action — send Telegram notification, sound a siren.
    break;
  }

  case DeviceType::WeatherNode:
  {
    if (len < (int)sizeof(WeatherMsg))
    {
      return;
    }
    WeatherMsg m;
    memcpy(&m, data, sizeof(m));
    logHeader(mac, h);
    if (isnan(m.humidityPct)) // BMP280 doesn't read humidity
    {
      Serial.printf("WEATHER %.1fC %.0fhPa (no humidity)\n", m.temperatureC, m.pressureHPa);
    }
    else
    {
      Serial.printf("WEATHER %.1fC %.0f%% %.0fhPa\n", m.temperatureC, m.humidityPct, m.pressureHPa);
    }

    if (weatherQueue)
    {
      xQueueSend(weatherQueue, &m, 0); // drop if the queue is full
    }
    break;
  }

  default:
    logHeader(mac, h);
    Serial.printf("UNKNOWN deviceType=%u len=%d\n", h.deviceType, len);
    break;
  }
}

static void postReading(const WeatherMsg &m)
{
  String url = weather_config::postUrl();
  if (url.length() == 0)
  {
    return;
  }
  if (!wifi_link::isConnected() && !wifi_link::connect())
  {
    Serial.println("POST skipped: WiFi unavailable");
    return;
  }

  String body = "{";
  body += "\"temperature\":" + String(m.temperatureC, 2);
  if (!isnan(m.humidityPct))
  {
    body += ",\"humidity\":" + String(m.humidityPct, 2);
  }
  body += ",\"pressure\":" + String(m.pressureHPa, 2);
  body += "}";

  WiFiClientSecure secure;
  WiFiClient plain;
  HTTPClient http;
  bool begun;
  if (url.startsWith("https"))
  {
    secure.setInsecure();
    begun = http.begin(secure, url);
  }
  else
  {
    begun = http.begin(plain, url);
  }
  if (!begun)
  {
    Serial.println("POST failed: http.begin()");
    return;
  }
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  Serial.printf("POST -> %d\n", code);
  http.end();
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

  weatherQueue = xQueueCreate(8, sizeof(WeatherMsg));

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


  WeatherMsg m;
  while (weatherQueue && xQueueReceive(weatherQueue, &m, 0) == pdTRUE)
  {
    postReading(m);
  }


  delay(20);
}
