// ESP-NOW master / gateway example.
//
// This file is guarded so it does NOT compile into the normal node firmware (which has its
// own setup()/loop() in main.cpp). Build it with the dedicated env:
//     pio run -e master
// (temporary home next to main.cpp — intended to be moved to its own project later.)

#ifdef ESPGUARD_MASTER

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "espnow_protocol.h"

using namespace espnow_protocol;

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
    if (len < (int)sizeof(AlarmMsg))
    {
      return;
    }
    AlarmMsg m;
    memcpy(&m, data, sizeof(m));
    logHeader(mac, h);
    Serial.printf("ALARM event=%u armed=%u door=%u triggers=%u\n",
                  h.event, m.armed, m.doorOpen, m.triggerCount);
    // TODO: take action — relay to Telegram, sound a siren, update a dashboard, etc.
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
    Serial.printf("WEATHER %.1fC %.0f%% %.0fhPa\n", m.temperatureC, m.humidityPct, m.pressureHPa);
    // TODO: act on weather (e.g. freeze/flood alerts) or feed cross-sensor logic.
    break;
  }

  default:
    logHeader(mac, h);
    Serial.printf("UNKNOWN deviceType=%u len=%d\n", h.deviceType, len);
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  WiFi.mode(WIFI_STA);
  Serial.print("Master MAC (enter this in each node's portal): ");
  Serial.println(WiFi.macAddress());

  // If the master also uses home WiFi for Telegram, its channel is the router's; the nodes
  // must be on that same channel. Otherwise pin it to match the nodes' MESH_CHANNEL:
  // esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(onReceive);
  Serial.println("Master ready — listening for ESP-NOW messages");
}

void loop()
{
  // All work happens in the receive callback; the master stays awake.
}

#endif // ESPGUARD_MASTER
