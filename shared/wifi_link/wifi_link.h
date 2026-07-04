#pragma once

#include <Arduino.h>

// Plain WiFi station connection shared by any device that needs the network
// (alarm Telegram fallback, orchestrator dashboard POST). Handles PSK and
// WPA2-Enterprise from the credentials store and applies the device MAC
// override. Deliberately free of any Telegram/HTTP dependency so lightweight
// consumers (e.g. the orchestrator) don't link them.
namespace wifi_link
{
  // Join WiFi using the stored credentials
  bool connect(unsigned long timeoutMs = 20000);

  bool isConnected();
}
