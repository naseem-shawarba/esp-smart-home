#include "web_device.h"

#include <Arduino.h>
#include <WebServer.h>
#include "device_config.h"

namespace web_device
{
  static WebServer *srv = nullptr;
  static const unsigned long MS_PER_MIN = 60UL * 1000UL;

  static String buildForm()
  {
    String html;
    html.reserve(1400);
    html += "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>Device settings</title></head>";
    html += "<body style='font-family:Verdana,sans-serif;font-size:14px;'>";
    html += "<div style='max-width:420px;padding:20px;border-radius:10px;border:solid 2px #e0e0e0;margin:20px auto;'>";
    html += "<div style='text-align:center;font-size:18px;font-weight:bold;margin-bottom:12px;'>Device settings</div>";
    html += "<form method='POST' action='/device/save'>";

    html += "<label>Device name<br><input name='name' value='" + device_config::name() + "'></label><br><br>";
    html += "<label>Node id (0-255)<br><input type='number' min='0' max='255' name='nodeId' value='" +
            String(device_config::nodeId()) + "'></label><br><br>";
    html += "<label>Portal window (minutes)<br><input type='number' min='1' name='portalMin' value='" +
            String(device_config::portalWindowMs() / MS_PER_MIN) + "'></label><br><br>";

    String macVal;
    if (device_config::hasMac())
    {
      uint8_t m[6];
      device_config::mac(m);
      macVal = device_config::formatMac(m);
    }
    html += "<label>MAC override (optional, e.g. AA:BB:CC:DD:EE:FF)<br><input name='mac' value='" +
            macVal + "' placeholder='(factory MAC)'></label><br>";
    html += "<label style='font-weight:normal;'><input type='checkbox' name='clearMac' value='1'> Clear (use factory MAC)</label><br><br>";

    html += "<input type='submit' value='Save' style='width:100%;padding:8px;'>";
    html += "</form>";
    html += "<div style='margin-top:14px;text-align:center;'><a href='/'>&larr; Menu</a></div>";
    html += "</div></body></html>";
    return html;
  }

  static void handleGet()
  {
    srv->send(200, "text/html", buildForm());
  }

  static void handleSave()
  {
    if (srv->hasArg("name") && srv->arg("name").length() > 0)
    {
      device_config::setName(srv->arg("name"));
    }
    if (srv->hasArg("nodeId"))
    {
      long id = srv->arg("nodeId").toInt();
      if (id >= 0 && id <= 255)
      {
        device_config::setNodeId((uint8_t)id);
      }
    }
    if (srv->hasArg("portalMin"))
    {
      long minutes = srv->arg("portalMin").toInt();
      if (minutes > 0)
      {
        device_config::setPortalWindowMs((unsigned long)minutes * MS_PER_MIN);
      }
    }
    // MAC: clear takes precedence; otherwise set only when a valid MAC was typed
    // (blank with no "clear" = leave unchanged).
    if (srv->hasArg("clearMac"))
    {
      device_config::clearMac();
    }
    else if (srv->hasArg("mac") && srv->arg("mac").length() > 0)
    {
      uint8_t macBytes[6];
      if (device_config::parseMac(srv->arg("mac"), macBytes))
      {
        device_config::setMac(macBytes);
      }
    }

    String html;
    html += "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>Saved</title></head>";
    html += "<body style='font-family:Verdana,sans-serif;font-size:14px;'>";
    html += "<div style='max-width:420px;padding:20px;border-radius:10px;border:solid 2px #e0e0e0;margin:20px auto;text-align:center;'>";
    html += "<div style='font-size:18px;font-weight:bold;margin-bottom:8px;'>Device settings saved</div>";
    html += "<p>The portal window applies to the next portal session.</p>";
    html += "<a href='/'>&larr; Menu</a>";
    html += "</div></body></html>";
    srv->send(200, "text/html", html);
  }

  void registerRoutes(WebServer &server)
  {
    srv = &server;
    server.on("/device", HTTP_GET, handleGet);
    server.on("/device/save", HTTP_POST, handleSave);
  }
}
