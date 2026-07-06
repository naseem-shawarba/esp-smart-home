#pragma once

#include <Arduino.h>

// Plain WiFi station connection shared by any device that needs the network
// (alarm Telegram fallback, orchestrator dashboard POST). Handles PSK and
// WPA2-Enterprise from the credentials store and applies the device MAC
// override. Deliberately free of any Telegram/HTTP dependency so lightweight
// consumers (e.g. the orchestrator) don't link them.
namespace wifi_link
{
  // Bring the STA driver up WITHOUT associating to an AP, and apply the MAC
  // override. Call once at boot when ESP-NOW must run alongside on-demand WiFi:
  // the driver stays up so ESP-NOW survives later associate()/disconnect() calls.
  void beginStation();

  // Join the stored AP (PSK or WPA2-Enterprise). Assumes beginStation() already
  // ran. It does NOT reset the stack, so ESP-NOW stays initialized. Blocks up
  // to timeoutMs; returns true if connected.
  bool associate(unsigned long timeoutMs = 20000);

  // Drop the AP link but keep STA mode up (ESP-NOW keeps running). After this the
  // caller can restore the ESP-NOW channel with esp_wifi_set_channel().
  void disconnect();

  // One-shot connect: fully resets the WiFi stack (WIFI_OFF), then associates.
  // Convenient when nothing else needs the radio (e.g. the alarm's fallback,
  // which sleeps afterwards). NOTE: the WIFI_OFF reset tears down ESP-NOW.
  bool connect(unsigned long timeoutMs = 20000);

  bool isConnected();
}
