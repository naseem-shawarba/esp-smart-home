#include "notifier.h"

#include <Arduino.h>
#include "settings.h"
#include "device_config.h"
#include "connectivity.h"
#include "alarm_text.h"
#include "mesh.h"

namespace notifier
{
  using espnow_protocol::AlarmEvent;

  // Event-specific duration carried alongside the event so the text (here or on
  // the orchestrator) can render "Arming in X". Only Armed uses one today.
  static unsigned long eventDetailMs(AlarmEvent event)
  {
    if (event == AlarmEvent::Armed)
    {
      return settings::get().postAlarmActivationDelayMs;
    }
    return 0;
  }

  void alarm(AlarmEvent event, bool armed, bool doorOpen, uint8_t triggerCount)
  {
    unsigned long detailMs = eventDetailMs(event);

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
    connectivity::sendMessage(alarm_text::eventText(event, doorOpen, detailMs));
  }
}
