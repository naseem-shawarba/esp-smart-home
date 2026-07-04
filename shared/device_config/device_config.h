#pragma once

#include <Arduino.h>

// Per-device identity + shared portal settings, persisted in the NVS "device" namespace and
// editable from the web portal. Defaults come from the DEVICE_DEFAULT_* build flags so each
// firmware ships a sensible name/id (e.g. "Door", "Orchestrator", "Weather").
namespace device_config
{
  String name();                        // shown in the portal title
  void setName(const String &value);

  uint8_t nodeId();                     // logical id the Orchestrator sees in ESP-NOW messages
  void setNodeId(uint8_t value);

  unsigned long portalWindowMs();       // how long the web portal stays open
  void setPortalWindowMs(unsigned long value);

  // Optional custom STA MAC. Part of the device's identity: it becomes the address the
  // device uses for both WiFi and ESP-NOW (see applyMacOverride).
  bool hasMac();
  void mac(uint8_t out[6]);
  void setMac(const uint8_t value[6]);
  void clearMac();

  // Apply the custom MAC to WIFI_IF_STA when one is set. Call once after WiFi.mode(WIFI_STA)
  // and before esp_now_init()/WiFi.begin() so the ESP-NOW and WiFi identities match and the
  // MAC never changes on a live interface. No-op when no custom MAC is set.
  void applyMacOverride();

  // MAC string helpers (shared with the credentials form for the Orchestrator-MAC field).
  bool parseMac(const String &in, uint8_t out[6]);  // "AA:BB:.." / "AABB.." -> 6 bytes
  String formatMac(const uint8_t value[6]);         // -> "AA:BB:CC:DD:EE:FF"

  // ESP-NOW mesh topology (non-secret operational config).
  bool isMeshMode();               // report to a Orchestrator over ESP-NOW instead of Telegram
  bool telegramFallbackEnabled();  // on ESP-NOW failure, fall back to WiFi+Telegram
  void setMesh(bool enabled, bool telegramFallback);
  bool hasOrchestratorMac();
  void orchestratorMac(uint8_t out[6]);  // the Orchestrator's MAC (ESP-NOW peer)
  void setOrchestratorMac(const uint8_t value[6]);
}
