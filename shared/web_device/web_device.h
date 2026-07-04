#pragma once

#include <WebServer.h>

// Shared portal page for device identity + portal settings (name, node id, portal window).
// Available on every device. Register after the server is created, before server.begin().
namespace web_device
{
  void registerRoutes(WebServer &server); // GET /device, POST /device/save
}
