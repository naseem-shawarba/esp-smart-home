#pragma once

#include <Arduino.h>
#include "espnow_protocol.h"

// Human-readable text for an alarm event, shared by the guard (its own Telegram
// path) and the orchestrator (relaying received GuardMsg). The event-specific
// duration (e.g. the arming countdown for Armed) is passed in rather than read
// from settings, so the orchestrator — which has no access to the guard's
// settings — renders identical text from the payload's detailMs.
namespace alarm_text
{
  String eventText(espnow_protocol::AlarmEvent event, bool doorOpen, unsigned long detailMs);
}
