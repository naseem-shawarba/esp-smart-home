#pragma once

#include <Arduino.h>
#include <WebServer.h>

// Generic setup/OTA web portal: SoftAP + captive DNS + a menu that always offers the
// credentials form, the device-settings page, and firmware update (ESP2SOTA). Devices
// customize it via Options — no dependency on any device-specific module.
namespace web_portal
{
  struct Options
  {
    String deviceName;                                  // shown in the menu/title
    unsigned long durationMs;                           // how long the portal stays open
    bool showMeshFields = false;                        // credentials form: mesh section?
    const char *extraMenuLabel = nullptr;               // optional device-specific menu entry
    const char *extraMenuHref = nullptr;                // e.g. "/settings"
    void (*registerExtraRoutes)(WebServer &) = nullptr; // device-specific pages (alarm settings)
    bool (*shouldExit)() = nullptr;                     // return true to close early (e.g. button)
    void (*onTick)() = nullptr;                         // per-loop hook (e.g. LED blink)
  };

  void run(const Options &opts);
}
