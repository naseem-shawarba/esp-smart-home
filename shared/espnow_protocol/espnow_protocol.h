#pragma once

// Shared ESP-NOW message contract between sensor nodes (e.g. the esp-guard alarm,
// a weather node) and the Orchestrator/gateway. BOTH the node firmware and the Orchestrator
// include this file so the binary layout matches exactly.
//
// Every message begins with `Header`. The Orchestrator reads `header.deviceType` to know
// which full struct the bytes are, then casts. Add a new sensor by adding a
// DeviceType + a struct that starts with `Header` — the Orchestrator just gains one case.

#include <stdint.h>

namespace espnow_protocol
{
  static const uint8_t PROTOCOL_VERSION = 1;
  static const uint8_t BATTERY_UNKNOWN = 255;

  // All devices must share one WiFi channel for ESP-NOW — part of the link contract.
  static const uint8_t MESH_CHANNEL = 6;

  enum class DeviceType : uint8_t
  {
    Unknown = 0,
    AlarmNode = 1,
    WeatherNode = 2,
    // add more sensor types here
  };

  // Common header at the start of EVERY message.
  struct __attribute__((packed)) Header
  {
    uint8_t version;    // == PROTOCOL_VERSION (reject mismatches)
    uint8_t deviceType; // DeviceType — tells the Orchestrator how to parse the body
    uint8_t nodeId;     // user-assigned logical id (distinguish two alarms, etc.)
    uint8_t event;      // per-device event code (enums below)
    uint32_t seq;       // increments per send — detect loss / duplicates
    uint32_t uptimeMs;  // millis() at send (rough ordering; Orchestrator adds real time)
    uint8_t batteryPct; // 0..100, or BATTERY_UNKNOWN
  };

  // ---- Alarm node ----
  enum class AlarmEvent : uint8_t
  {
    Armed = 1,
    Disarmed,
    Triggered,     // door opened while armed, disarm window expired
    DoorState,     // post-grace door report
    TempDisabled,  // auto-disabled after too many triggers
    ReArmed,       // re-armed after temporary disable
    PortalEntered, // went into config/OTA mode
    Heartbeat,     // periodic "I'm alive"
  };

  struct __attribute__((packed)) GuardMsg
  {
    Header header;
    uint8_t armed;        // 0/1
    uint8_t doorOpen;     // 0/1
    uint8_t triggerCount; // triggers since arming
    uint32_t detailMs;    // event-specific duration (e.g. arming countdown for Armed); 0 if n/a
  };

  // ---- Weather node (example of a second sensor type) ----
  enum class WeatherEvent : uint8_t
  {
    Reading = 1,
  };

  struct __attribute__((packed)) WeatherMsg
  {
    Header header;
    float temperatureC;
    float humidityPct;
    float pressureHPa;
  };
}
