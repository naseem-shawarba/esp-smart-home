#include "wifi_link.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wpa2.h>
#include "credentials.h"
#include "device_config.h"

namespace wifi_link
{
  void beginStation()
  {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false); // we manage association manually (channel hopping)
    WiFi.persistent(false);
    // Same identity for WiFi and ESP-NOW (no-op when no custom MAC is set).
    device_config::applyMacOverride();
  }

  bool associate(unsigned long timeoutMs)
  {
    credentials::WifiCreds w = credentials::wifi();
    if (w.type == credentials::WifiType::Enterprise)
    {
      Serial.println("Connecting to WPA2-Enterprise WiFi...");
      WiFi.begin(w.ssid.c_str());
      esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)w.eapIdentity.c_str(), w.eapIdentity.length());
      esp_wifi_sta_wpa2_ent_set_username((uint8_t *)w.eapUsername.c_str(), w.eapUsername.length());
      esp_wifi_sta_wpa2_ent_set_password((uint8_t *)w.eapPassword.c_str(), w.eapPassword.length());
      esp_wifi_sta_wpa2_ent_enable();
    }
    else
    {
      Serial.println("Connecting to WiFi (WPA2-PSK)...");
      WiFi.begin(w.ssid.c_str(), w.password.c_str());
    }

    // Bounded wait so a misconfigured device doesn't hang forever.
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs)
    {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.print("\nWiFi Connected! IP: ");
      Serial.print(WiFi.localIP());
      Serial.printf("  (channel %d)\n", WiFi.channel());
      return true;
    }
    Serial.println("\nWiFi connect timed out");
    return false;
  }

  void disconnect()
  {
    WiFi.disconnect(false); // drop the AP link, keep the STA driver (and ESP-NOW) up
  }

  bool connect(unsigned long timeoutMs)
  {
    // Fully restart the WiFi stack for a clean join (tears down ESP-NOW).
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    beginStation();
    return associate(timeoutMs);
  }

  bool isConnected()
  {
    return WiFi.status() == WL_CONNECTED;
  }
}
