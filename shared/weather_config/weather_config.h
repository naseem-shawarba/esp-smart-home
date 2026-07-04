#pragma once

#include <Arduino.h>
#include <WebServer.h>

// Shared "Settings" portal page + NVS store for the POST endpoint URL — the
// HTTP(S) endpoint sensor readings are POSTed to. The Orchestrator uses it to POST;
// other devices can expose it too via their portal. Defaults to the
// DEFAULT_POST_URL build flag (empty = POSTing disabled). The URL lives in NVS,
// never in git.
namespace weather_config
{
  String postUrl();
  void setPostUrl(const String &value);

  void registerRoutes(WebServer &server);
}
