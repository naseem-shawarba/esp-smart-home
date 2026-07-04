#include "mesh.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "device_config.h"

namespace mesh
{
  using namespace espnow_protocol;

  static volatile bool sendDone = false;
  static volatile bool sendOk = false;
  static bool initialized = false;

  // Persisted across deep sleep so the orchestrator sees a monotonic sequence.
  RTC_DATA_ATTR static uint32_t seqCounter = 0;

  static void onSent(const uint8_t *mac, esp_now_send_status_t status)
  {
    sendOk = (status == ESP_NOW_SEND_SUCCESS);
    sendDone = true;
  }

  void begin()
  {
    if (initialized)
    {
      return;
    }

    WiFi.mode(WIFI_STA);
    device_config::applyMacOverride(); // match WiFi/ESP-NOW identity before ESP-NOW starts
    esp_wifi_set_channel(MESH_CHANNEL, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK)
    {
      Serial.println("ESP-NOW init failed");
      return;
    }
    esp_now_register_send_cb(onSent);

    esp_now_peer_info_t peer = {};
    device_config::orchestratorMac(peer.peer_addr);
    peer.channel = MESH_CHANNEL;
    peer.encrypt = false;
    esp_now_add_peer(&peer);

    initialized = true;
  }

  bool sendAlarm(AlarmEvent event, bool armed, bool doorOpen, uint8_t triggerCount)
  {
    begin();

    uint8_t orchestratorMac[6];
    device_config::orchestratorMac(orchestratorMac);

    GuardMsg m = {};
    m.header.version = PROTOCOL_VERSION;
    m.header.deviceType = (uint8_t)DeviceType::AlarmNode;
    m.header.nodeId = device_config::nodeId();
    m.header.event = (uint8_t)event;
    m.header.seq = ++seqCounter;
    m.header.uptimeMs = millis();
    m.header.batteryPct = BATTERY_UNKNOWN;
    m.armed = armed ? 1 : 0;
    m.doorOpen = doorOpen ? 1 : 0;
    m.triggerCount = triggerCount;

    const int maxAttempts = 3;
    for (int attempt = 0; attempt < maxAttempts; attempt++)
    {
      sendDone = false;
      sendOk = false;

      if (esp_now_send(orchestratorMac, (uint8_t *)&m, sizeof(m)) != ESP_OK)
      {
        delay(20);
        continue;
      }

      unsigned long start = millis();
      while (!sendDone && millis() - start < 100)
      {
        delay(2);
      }

      if (sendDone && sendOk)
      {
        Serial.printf("ESP-NOW sent (event %u, seq %u)\n", (unsigned)event, (unsigned)m.header.seq);
        return true;
      }
    }

    Serial.println("ESP-NOW send failed");
    return false;
  }
}
