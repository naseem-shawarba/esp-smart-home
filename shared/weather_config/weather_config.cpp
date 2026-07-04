#include "weather_config.h"

#include <Preferences.h>

#ifndef DEFAULT_POST_URL
#define DEFAULT_POST_URL ""
#endif

namespace weather_config
{
  static const char *NVS_NAMESPACE = "weather_config";
  static const char *KEY_POST_URL = "postUrl";
  static WebServer *srv = nullptr;

  String postUrl()
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    String v = prefs.getString(KEY_POST_URL, DEFAULT_POST_URL);
    prefs.end();
    return v;
  }

  void setPostUrl(const String &value)
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString(KEY_POST_URL, value);
    prefs.end();
  }

  static String buildForm()
  {
    String html;
    html.reserve(1200);
    html += "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>Settings</title></head>";
    html += "<body style='font-family:Verdana,sans-serif;font-size:14px;'>";
    html += "<div style='max-width:420px;padding:20px;border-radius:10px;border:solid 2px #e0e0e0;margin:20px auto;'>";
    html += "<div style='text-align:center;font-size:18px;font-weight:bold;margin-bottom:12px;'>Settings</div>";
    html += "<form method='POST' action='/settings/save'>";
    html += "<label>POST URL (server endpoint)<br><input name='postUrl' value='" + postUrl() +
            "' placeholder='https://host/api/data'></label><br>";
    html += "<div style='color:#666;margin:6px 0 14px;'>Where sensor readings are POSTed as JSON. Leave empty to disable.</div>";
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
    if (srv->hasArg("postUrl"))
    {
      setPostUrl(srv->arg("postUrl"));
    }

    String html;
    html += "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>Saved</title></head>";
    html += "<body style='font-family:Verdana,sans-serif;font-size:14px;'>";
    html += "<div style='max-width:420px;padding:20px;border-radius:10px;border:solid 2px #e0e0e0;margin:20px auto;text-align:center;'>";
    html += "<div style='font-size:18px;font-weight:bold;margin-bottom:8px;'>Settings saved</div>";
    html += "<a href='/'>&larr; Menu</a>";
    html += "</div></body></html>";
    srv->send(200, "text/html", html);
  }

  void registerRoutes(WebServer &server)
  {
    srv = &server;
    server.on("/settings", HTTP_GET, handleGet);
    server.on("/settings/save", HTTP_POST, handleSave);
  }
}
