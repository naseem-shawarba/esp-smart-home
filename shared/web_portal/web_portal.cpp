#include "web_portal.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESP2SOTA.h>
#include "credentials.h"
#include "web_credentials.h"
#include "web_device.h"

namespace web_portal
{
  static WebServer server(80);
  static DNSServer dnsServer;
  static Options opts_;

  static String menuHtml()
  {
    String title = opts_.deviceName.length() ? opts_.deviceName : String("ESP-Guard");
    String html;
    html += "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>" + title + "</title></head>";
    html += "<body style='font-family:Verdana,sans-serif;font-size:15px;'>";
    html += "<div style='max-width:420px;padding:20px;border-radius:10px;border:solid 2px #e0e0e0;margin:20px auto;'>";
    html += "<div style='text-align:center;font-size:20px;font-weight:bold;margin-bottom:16px;'>" + title + "</div>";
    html += "<p><a href='/credentials'>&#128272; Configure WiFi &amp; Telegram</a></p>";
    html += "<p><a href='/device'>&#9881; Device settings</a></p>";
    if (opts_.extraMenuLabel && opts_.extraMenuHref)
    {
      html += "<p><a href='" + String(opts_.extraMenuHref) + "'>" + String(opts_.extraMenuLabel) + "</a></p>";
    }
    html += "<p><a href='/update'>&#11014; Firmware update</a></p>";
    html += "</div></body></html>";
    return html;
  }

  static void handleMenu()
  {
    server.send(200, "text/html", menuHtml());
  }

  // Captive-portal: send any unknown URL to the menu so the OS sign-in page pops up.
  static void handleNotFound()
  {
    server.sendHeader("Location", "http://10.10.10.1/", true);
    server.send(302, "text/plain", "");
  }

  void run(const Options &opts)
  {
    opts_ = opts;

    delay(1000);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(credentials::apSsid().c_str(), credentials::apPassword().c_str());
    delay(1000);
    IPAddress IP = IPAddress(10, 10, 10, 1);
    IPAddress NMask = IPAddress(255, 255, 255, 0);
    WiFi.softAPConfig(IP, IP, NMask);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    // Captive portal: resolve every hostname to us so the menu auto-opens on connect.
    dnsServer.start(53, "*", IP);

    ESP2SOTA.begin(&server);
    server.on("/", HTTP_GET, handleMenu);
    web_credentials::registerRoutes(server, opts.showMeshFields);
    web_device::registerRoutes(server);
    if (opts.registerExtraRoutes)
    {
      opts.registerExtraRoutes(server);
    }
    server.onNotFound(handleNotFound);
    server.begin();

    delay(1000);
    unsigned long startTime = millis();
    while (millis() - startTime < opts.durationMs)
    {
      if (opts.onTick)
      {
        opts.onTick();
      }
      if (opts.shouldExit && opts.shouldExit())
      {
        delay(2500);
        break;
      }
      dnsServer.processNextRequest();
      server.handleClient();
      delay(5);
    }

    dnsServer.stop();
    Serial.println("Portal window ended");
  }
}
