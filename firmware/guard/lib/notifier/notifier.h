#pragma once

#include "espnow_protocol.h"

// Mode-aware reporting of alarm events. Keeps alarm logic transport-agnostic:
// - standalone mode -> WiFi + Telegram (human text)
// - mesh mode        -> ESP-NOW to the orchestrator, with optional Telegram fallback
namespace notifier
{
  void alarm(espnow_protocol::AlarmEvent event, bool armed, bool doorOpen, uint8_t triggerCount);
}
