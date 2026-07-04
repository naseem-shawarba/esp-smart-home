#include "device_config.h"

#include <Preferences.h>
#include <esp_wifi.h>

// Per-firmware defaults (set via build_flags). Fallbacks keep the shared lib self-contained.
#ifndef DEVICE_DEFAULT_NAME
#define DEVICE_DEFAULT_NAME "device"
#endif
#ifndef DEVICE_DEFAULT_NODE_ID
#define DEVICE_DEFAULT_NODE_ID 1
#endif
#ifndef DEVICE_DEFAULT_PORTAL_WINDOW_MS
#define DEVICE_DEFAULT_PORTAL_WINDOW_MS (5UL * 60 * 1000)
#endif

namespace device_config
{
  static const char *NVS_NAMESPACE = "device";
  static const char *KEY_NAME = "name";
  static const char *KEY_NODE_ID = "nodeId";
  static const char *KEY_PORTAL_MS = "portalMs";
  static const char *KEY_MAC = "newMACAddress";
  static const char *KEY_MESH = "meshMode";
  static const char *KEY_TG_FALLBACK = "tgFallback";
  static const char *KEY_ORCHEST_MAC = "orchestratorMac";

  String name()
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    String v = prefs.getString(KEY_NAME, DEVICE_DEFAULT_NAME);
    prefs.end();
    return v;
  }

  void setName(const String &value)
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString(KEY_NAME, value);
    prefs.end();
  }

  uint8_t nodeId()
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t v = prefs.getUChar(KEY_NODE_ID, DEVICE_DEFAULT_NODE_ID);
    prefs.end();
    return v;
  }

  void setNodeId(uint8_t value)
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(KEY_NODE_ID, value);
    prefs.end();
  }

  unsigned long portalWindowMs()
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    unsigned long v = prefs.getULong(KEY_PORTAL_MS, DEVICE_DEFAULT_PORTAL_WINDOW_MS);
    prefs.end();
    return v;
  }

  void setPortalWindowMs(unsigned long value)
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putULong(KEY_PORTAL_MS, value);
    prefs.end();
  }

  bool hasMac()
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    bool present = prefs.getBytesLength(KEY_MAC) == 6;
    prefs.end();
    return present;
  }

  void mac(uint8_t out[6])
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    prefs.getBytes(KEY_MAC, out, 6);
    prefs.end();
  }

  void setMac(const uint8_t value[6])
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBytes(KEY_MAC, value, 6);
    prefs.end();
  }

  void clearMac()
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.remove(KEY_MAC);
    prefs.end();
  }

  void applyMacOverride()
  {
    if (!hasMac())
    {
      return;
    }
    uint8_t macBytes[6];
    mac(macBytes);
    esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, macBytes);
    Serial.println(err == ESP_OK ? "Success changing MAC Address" : "Failed to set MAC");
  }

  bool parseMac(const String &in, uint8_t out[6])
  {
    String hex;
    for (size_t i = 0; i < in.length(); i++)
    {
      char c = in[i];
      if (isxdigit(c))
      {
        hex += c;
      }
    }
    if (hex.length() != 12)
    {
      return false;
    }
    for (int i = 0; i < 6; i++)
    {
      out[i] = (uint8_t)strtoul(hex.substring(i * 2, i * 2 + 2).c_str(), nullptr, 16);
    }
    return true;
  }

  String formatMac(const uint8_t value[6])
  {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             value[0], value[1], value[2], value[3], value[4], value[5]);
    return String(buf);
  }

  bool isMeshMode()
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    bool v = prefs.getBool(KEY_MESH, false);
    prefs.end();
    return v;
  }

  bool telegramFallbackEnabled()
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    bool v = prefs.getBool(KEY_TG_FALLBACK, false);
    prefs.end();
    return v;
  }

  void setMesh(bool enabled, bool telegramFallback)
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool(KEY_MESH, enabled);
    prefs.putBool(KEY_TG_FALLBACK, telegramFallback);
    prefs.end();
  }

  bool hasOrchestratorMac()
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    bool present = prefs.getBytesLength(KEY_ORCHEST_MAC) == 6;
    prefs.end();
    return present;
  }

  void orchestratorMac(uint8_t out[6])
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    prefs.getBytes(KEY_ORCHEST_MAC, out, 6);
    prefs.end();
  }

  void setOrchestratorMac(const uint8_t value[6])
  {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBytes(KEY_ORCHEST_MAC, value, 6);
    prefs.end();
  }
}
