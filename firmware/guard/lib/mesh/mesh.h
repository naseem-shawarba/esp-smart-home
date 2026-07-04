#pragma once

#include "espnow_protocol.h"

// ESP-NOW sender for mesh mode
namespace mesh
{
  void begin(); // WIFI_STA + fixed channel + esp_now_init + add orchestrator peer (MAC from NVS)

  // Send one alarm event. Returns true only if the ESP-NOW send-status callback
  // reported delivery (peer ACKed); false lets the caller fall back to Telegram.
  bool sendAlarm(espnow_protocol::AlarmEvent event, bool armed, bool doorOpen, uint8_t triggerCount);
}
