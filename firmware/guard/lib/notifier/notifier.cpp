#include "notifier.h"

#include <Arduino.h>
#include "settings.h"
#include "device_config.h"
#include "connectivity.h"
#include "mesh.h"

namespace notifier
{
  using espnow_protocol::AlarmEvent;

  static String formatDuration(unsigned long ms)
  {
    if (ms >= 60000UL)
    {
      return String(ms / 60000UL) + " min";
    }
    return String(ms / 1000UL) + " sec";
  }

  // Human-readable text for the Telegram path (standalone or mesh fallback).
  static String eventText(AlarmEvent event, bool doorOpen, unsigned long detailMs)
  {
    switch (event)
    {
    case AlarmEvent::Armed:
    {
      String s = "Alarm activated.";
      if (detailMs > 0)
      {
        s += " Arming in " + formatDuration(detailMs) + ".";
      }
      s += doorOpen ? " Door is open." : " Door is closed.";
      return s;
    }
    case AlarmEvent::Disarmed:
      return "Alarm deactivated";
    case AlarmEvent::Triggered:
      return "The Door has been opened";
    case AlarmEvent::DoorState:
      return doorOpen ? "Door is open" : "Door is closed";
    case AlarmEvent::TempDisabled:
      return "The alarm has been temporarily deactivated";
    case AlarmEvent::ReArmed:
      return "The alarm has been re-activated after temporal deactivation";
    case AlarmEvent::PortalEntered:
      return "Exiting portal mode";
    default:
      return "Alarm event";
    }
  }

  void alarm(AlarmEvent event, bool armed, bool doorOpen, uint8_t triggerCount)
  {
    unsigned long detailMs = settings::get().postAlarmActivationDelayMs;

    if (device_config::isMeshMode())
    {
      if (mesh::sendAlarm(event, armed, doorOpen, triggerCount, detailMs))
      {
        return; // delivered to the orchestrator, which renders + relays the text
      }
      if (!device_config::telegramFallbackEnabled())
      {
        Serial.println("Mesh send failed; Telegram fallback disabled");
        return;
      }
      Serial.println("Mesh send failed; falling back to Telegram");
    }

    connectivity::connectWiFi();
    connectivity::sendMessage(eventText(event, doorOpen, detailMs));
  }
}
