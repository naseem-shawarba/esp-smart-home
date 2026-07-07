// ESP-Guard simple guard node (ESP32-C3) battery door sensor.
//
// Deep-sleeps until the reed switch reports the door opened, delivers one
// AlarmEvent::Triggered, then sleeps again. Two delivery modes (set by the
// portal's mesh checkbox):
//   - Mesh (device_config::isMeshMode()): send to the orchestrator over ESP-NOW;
//     the orchestrator does the Telegram notification.
//   - Standalone: connect to WiFi and send the Telegram message directly.
//
// The setup portal opens on first boot / on demand (BOOT button), or whenever
// the ACTIVE mode isn't configured (mesh: no orchestrator MAC; standalone: no
// WiFi + Telegram). It reuses the shared portal: credentials (WiFi/Telegram +
// mesh/orchestrator MAC), device settings, OTA.
//
// Wiring: reed switch between DOOR_PIN and GND. An external pull resistor to the
// "closed" level is recommended for reliable deep-sleep wake (the internal
// pull-up is enabled as a convenience but may not hold through deep sleep).

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include "espnow_protocol.h"
#include "credentials.h"
#include "device_config.h"
#include "wifi_link.h"
#include "connectivity.h"
#include "alarm_text.h"
#include "web_portal.h"

#if GUARD_OLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif

using namespace espnow_protocol;

#ifndef DOOR_PIN
#define DOOR_PIN 3
#endif

#ifndef PORTAL_PIN
#define PORTAL_PIN -1
#endif

#ifndef BUILTIN_LED_PIN
#define BUILTIN_LED_PIN -1
#endif

// How long to wait for the door to close before sleeping, so a still-open door
// doesn't immediately re-wake us in a loop.
#ifndef DOOR_CLOSE_TIMEOUT_MS
#define DOOR_CLOSE_TIMEOUT_MS 30000 // 30 sec
#endif

#if GUARD_OLED
#ifndef GUARD_OLED_WIDTH
#define GUARD_OLED_WIDTH 128
#endif
#ifndef GUARD_OLED_HEIGHT
#define GUARD_OLED_HEIGHT 64
#endif
#ifndef GUARD_OLED_ADDR
#define GUARD_OLED_ADDR 0x3C
#endif
static Adafruit_SSD1306 display(GUARD_OLED_WIDTH, GUARD_OLED_HEIGHT, &Wire, -1);
static bool oledReady = false;
#endif

RTC_DATA_ATTR static uint32_t seqCounter = 0;

static volatile bool sendDone = false;
static volatile bool sendOk = false;

static void onSent(const uint8_t *, esp_now_send_status_t status)
{
  sendOk = (status == ESP_NOW_SEND_SUCCESS);
  sendDone = true;
}

static bool doorIsOpen()
{
  return digitalRead(DOOR_PIN) == HIGH;
}

static void oledShow(const String &l1, const String &l2, const String &l3 = "")
{
#if GUARD_OLED
  if (!oledReady)
  {
    return;
  }
  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("=== ESP GUARD ===");
  display.println("");
  display.println(l1);
  display.println(l2);
  if (l3.length())
  {
    display.println(l3);
  }
  display.display();
#else
  (void)l1;
  (void)l2;
  (void)l3;
#endif
}

static bool espnowBegin()
{
  WiFi.mode(WIFI_STA);
  device_config::applyMacOverride();
  esp_wifi_set_channel(MESH_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW init failed");
    return false;
  }
  esp_now_register_send_cb(onSent);

  uint8_t orchestrator[6];
  device_config::orchestratorMac(orchestrator);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, orchestrator, 6);
  peer.channel = MESH_CHANNEL;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  return true;
}

static bool sendTriggered()
{
  uint8_t orchestrator[6];
  device_config::orchestratorMac(orchestrator);

  GuardMsg m = {};
  m.header.version = PROTOCOL_VERSION;
  m.header.deviceType = (uint8_t)DeviceType::AlarmNode;
  m.header.nodeId = device_config::nodeId();
  m.header.event = (uint8_t)AlarmEvent::Triggered;
  m.header.seq = ++seqCounter;
  m.header.uptimeMs = millis();
  m.header.batteryPct = BATTERY_UNKNOWN;
  m.armed = 1;
  m.doorOpen = 1;
  m.triggerCount = 1;
  m.detailMs = 0;

  for (int attempt = 0; attempt < 7; attempt++)
  {
    sendDone = false;
    sendOk = false;
    if (esp_now_send(orchestrator, (uint8_t *)&m, sizeof(m)) != ESP_OK)
    {
      delay(20);
      continue;
    }
    unsigned long start = millis();
    while (!sendDone && millis() - start < 100)
    {
      delay(2);
    }
    if (sendDone && sendOk)
    {
      Serial.printf("Triggered sent (seq %u)\n", (unsigned)m.header.seq);
      oledShow("ALARM TRIGGERED!", "Sent to Gateway", "Seq: " + String(m.header.seq));
      return true;
    }
  }
  Serial.println("Send to orchestrator failed (no fallback)");
  oledShow("ALARM TRIGGERED!", "Send FAILED", "No gateway link");
  return false;
}

static bool sendTelegram()
{
  if (!wifi_link::connect())
  {
    Serial.println("WiFi connect failed; cannot notify");
    oledShow("ALARM TRIGGERED!", "WiFi FAILED", "");
    return false;
  }
  String text = alarm_text::eventText(AlarmEvent::Triggered, /*doorOpen=*/true, /*detailMs=*/0);
  bool ok = connectivity::sendMessage(text);
  oledShow("ALARM TRIGGERED!", ok ? "Telegram sent" : "Telegram FAILED", "");
  return ok;
}

static bool notifyTriggered()
{
  if (device_config::isMeshMode())
  {
    return espnowBegin() && sendTriggered();
  }
  return sendTelegram();
}

static bool configured()
{
  if (device_config::isMeshMode())
  {
    return device_config::hasOrchestratorMac();
  }
  return credentials::hasWifi() && credentials::hasTelegram();
}

static void tick()
{
#if BUILTIN_LED_PIN >= 0
  digitalWrite(BUILTIN_LED_PIN, (millis() % 1500 > 750) ? HIGH : LOW);
#endif
}

static bool portalButtonClicked()
{
#if PORTAL_PIN >= 0
  return digitalRead(PORTAL_PIN) == LOW;
#else
  return false;
#endif
}

static void openPortal()
{
  web_portal::Options opts;
  opts.deviceName = device_config::name(); // defaults to "Simple Guard"
  opts.durationMs = device_config::portalWindowMs();
  opts.showMeshFields = true; // set the orchestrator MAC + mesh mode here
  opts.onTick = tick;
  opts.shouldExit = portalButtonClicked;
  web_portal::run(opts);
}

static void sleepUntilDoorOpens()
{
  unsigned long start = millis();
  while (doorIsOpen() && millis() - start < DOOR_CLOSE_TIMEOUT_MS)
  {
    delay(50);
  }

#if GUARD_OLED
  if (oledReady)
  {
    delay(2000); 
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }
#endif

  esp_deep_sleep_enable_gpio_wakeup(1ULL << DOOR_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH );
  Serial.println("Sleeping until the door opens");
  Serial.flush();
  esp_deep_sleep_start();
}

void setup()
{
  Serial.begin(115200);
  delay(100);

  pinMode(DOOR_PIN, INPUT_PULLUP);
#if PORTAL_PIN >= 0
  pinMode(PORTAL_PIN, INPUT_PULLUP);
#endif
#if BUILTIN_LED_PIN >= 0
  pinMode(BUILTIN_LED_PIN, OUTPUT);
#endif
  bool portalHeld = portalButtonClicked();

#if GUARD_OLED

  Wire.begin(GUARD_SDA, GUARD_SCL);
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, GUARD_OLED_ADDR);
  if (!oledReady)
  {
    Serial.println("OLED init failed");
  }
#endif

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

  if (portalHeld || !configured())
  {
    oledShow("Setup mode", "AP: " + credentials::apSsid());
    Serial.printf("Opening setup portal (AP %s)\n", credentials::apSsid().c_str());
    openPortal();
    ESP.restart(); // re-evaluate cleanly with the new config
  }

  // Only report when the door actually woke us; a power-on/reset just re-arms.
  if (cause == ESP_SLEEP_WAKEUP_GPIO)
  {
    Serial.println("Woke on door open");
    notifyTriggered();
  }
  else
  {
    Serial.println("Power-on/reset; arming door wake");
    oledShow("System Armed", "Status: Secure", "Waiting for door...");
  }

  sleepUntilDoorOpens();
}

void loop()
{
}
