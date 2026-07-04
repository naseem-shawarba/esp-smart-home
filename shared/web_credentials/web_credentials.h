#pragma once

#include <WebServer.h>

// Serves the credentials form (WiFi + Telegram + setup-AP + optional MAC, and — for nodes —
// the ESP-NOW mesh section) on the portal. Register after the server is created, before
// server.begin(). Set showMesh=true on sensor nodes, false on the orchestrator.
namespace web_credentials
{
  void registerRoutes(WebServer &server, bool showMesh); // GET /credentials, POST /credentials/save
}
