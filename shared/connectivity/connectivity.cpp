#include "connectivity.h"

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "credentials.h"
#include "wifi_link.h"

namespace connectivity
{
  // One secure client shared across messages.
  static WiFiClientSecure client;

  void connectWiFi()
  {
    wifi_link::connect();
    // Required for the HTTPS Telegram connection.
    client.setInsecure();
  }

  bool sendMessage(const String &message)
  {
    client.setInsecure(); // needed for the HTTPS Telegram connection (callers may skip connectWiFi)

    delay(100);
    String token = credentials::botToken();
    String chat = credentials::chatId();
    delay(100);

    UniversalTelegramBot bot(token.c_str(), client);
    Serial.print("Sending message: ");
    Serial.println(message);

    bool sent = bot.sendMessage(chat, message, "");
    Serial.println(sent ? "Message sent successfully" : "Message failed");
    delay(50);
    return sent;
  }
}
