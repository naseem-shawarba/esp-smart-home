#include "notifier.h"

#include <Arduino.h>
#include "settings.h"
#include "credentials.h"
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
  static String eventText(AlarmEvent event, bool doorOpen)
  {
    switch (event)
    {
    case AlarmEvent::Armed:
    {
      unsigned long buffer = settings::get().postAlarmActivationDelayMs;
      String s = "Alarm activated.";
      if (buffer > 0)
      {
        s += " Arming in " + formatDuration(buffer) + ".";
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
    if (credentials::isMeshMode())
    {
      if (mesh::sendAlarm(event, armed, doorOpen, triggerCount))
      {
        return; // delivered to the master
      }
      if (!credentials::telegramFallbackEnabled())
      {
        Serial.println("Mesh send failed; Telegram fallback disabled");
        return;
      }
      Serial.println("Mesh send failed; falling back to Telegram");
    }

    connectivity::connectWiFi();
    connectivity::sendMessage(eventText(event, doorOpen));
  }
}
