#pragma once

#include "espnow_protocol.h"

// ESP-NOW sender for mesh mode: the node fires a small packet at the master and
// deep-sleeps. Much cheaper than a WiFi+Telegram round-trip.
namespace mesh
{
  void begin(); // WIFI_STA + fixed channel + esp_now_init + add master peer (MAC from NVS)

  // Send one alarm event. Returns true only if the ESP-NOW send-status callback
  // reported delivery (peer ACKed); false lets the caller fall back to Telegram.
  bool sendAlarm(espnow_protocol::AlarmEvent event, bool armed, bool doorOpen, uint8_t triggerCount);
}
