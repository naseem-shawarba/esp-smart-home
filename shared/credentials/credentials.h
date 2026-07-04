#pragma once

#include <Arduino.h>

// Single source of truth for the NVS "credentials" namespace: the device's secrets —
// WiFi (PSK or WPA2-Enterprise), Telegram, and the setup-AP credentials. Non-secret
// device config (identity, own MAC, ESP-NOW mesh topology) lives in device_config.
// Provisioned at runtime via the web portal; nothing is hard-coded in the firmware.
namespace credentials
{
  enum class WifiType
  {
    Psk = 0,    // normal home WiFi (SSID + password)
    Enterprise, // WPA2-Enterprise (EAP identity/username/password)
  };

  struct WifiCreds
  {
    WifiType type;
    String ssid;
    String password;     // PSK
    String eapIdentity;  // Enterprise
    String eapUsername;  // Enterprise
    String eapPassword;  // Enterprise
  };

  // Reads
  WifiCreds wifi();
  String botToken();
  String chatId();
  String apSsid();     // OTA_SSID, or DEFAULT_AP_SSID when unset
  String apPassword(); // OTA_PASSWORD, or DEFAULT_AP_PASSWORD when unset

  // True when there's enough to operate for the current mode (mesh topology lives in
  // device_config; this reads it to decide which secrets are required).
  bool isConfigured();

  // Writes (used by the credentials form)
  void setWifi(const WifiCreds &creds);
  void setTelegram(const String &token, const String &chatId);
  void setAp(const String &ssid, const String &password);
}
