#include "alarm_text.h"

namespace alarm_text
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

  String eventText(AlarmEvent event, bool doorOpen, unsigned long detailMs)
  {
    switch (event)
    {
    case AlarmEvent::Armed:
    {
      String s = "🔒 Alarm activated.";
      if (detailMs > 0)
      {
        s += " ⏳ Arming in " + formatDuration(detailMs) + ".";
      }
      s += doorOpen ? " 🚪o Door is open." : " 🚪c Door is closed.";
      return s;
    }
    case AlarmEvent::Disarmed:
      return "🔓 Alarm deactivated";
    case AlarmEvent::Triggered:
      return "⚠️ The Door has been opened";
    case AlarmEvent::DoorState:
      return doorOpen ? "🚪o Door is open" : "🚪c Door is closed";
    case AlarmEvent::TempDisabled:
      return "⏸️ The alarm has been temporarily deactivated";
    case AlarmEvent::ReArmed:
      return "🔒 The alarm has been re-activated after temporal deactivation";
    case AlarmEvent::PortalEntered:
      return "🌀 Exiting portal mode";
    default:
      return "🔔 Alarm event";
    }}
  }
