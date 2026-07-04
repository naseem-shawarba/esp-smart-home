// ESP-Guard weather node (ESP32-C3).
// Auto-detects a BMP280/BME280, sends the reading to the Orchestrator over ESP-NOW
// The ESP#@ shows it on an OLED,
// then deep-sleeps.
// Notify-only: there is NO Telegram/direct-POST fallback.
//
// First boot (or with no Orchestrator MAC provisioned) opens a setup portal — reuse
// the shared portal: credentials (mesh -> Orchestrator MAC), device settings, OTA,
// and the "Settings" page. Set WEATHER_SLEEP_SEC=0 to stay awake (loop) instead
// of deep-sleeping, e.g. as a bench display.

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "espnow_protocol.h"
#include "credentials.h"
#include "device_config.h"
#include "web_portal.h"
#include "weather_config.h"
#include "weather_sensor.h"

#if WEATHER_OLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif

using namespace espnow_protocol;

// Deep sleep interval between readings, in seconds. 0 = never sleep (endless loop ).
#ifndef WEATHER_SLEEP_SEC
#define WEATHER_SLEEP_SEC 600
#endif
#ifndef WEATHER_AWAKE_INTERVAL_MS
#define WEATHER_AWAKE_INTERVAL_MS 10000
#endif
// Optional GPIO that opens the setup portal when held at boot (-1 = disabled).
#ifndef PORTAL_PIN
#define PORTAL_PIN -1
#endif

#ifndef BUILTIN_LED_PIN
#define BUILTIN_LED_PIN -1
#endif

#if WEATHER_OLED
#ifndef WEATHER_OLED_WIDTH
#define WEATHER_OLED_WIDTH 128
#endif
#ifndef WEATHER_OLED_HEIGHT
#define WEATHER_OLED_HEIGHT 64
#endif
#ifndef WEATHER_OLED_ADDR
#define WEATHER_OLED_ADDR 0x3C
#endif
static Adafruit_SSD1306 display(WEATHER_OLED_WIDTH, WEATHER_OLED_HEIGHT, &Wire, -1);
static bool oledReady = false;
#endif

// Sequence counter persists across deep sleep so the Orchestrator sees a monotonic series.
RTC_DATA_ATTR static uint32_t seqCounter = 0;

static volatile bool sendDone = false;
static volatile bool sendOk = false;

static void onSent(const uint8_t *, esp_now_send_status_t status)
{
  sendOk = (status == ESP_NOW_SEND_SUCCESS);
  sendDone = true;
}

static void oledShow(const String &l1, const String &l2, const String &l3 = "")
{
#if WEATHER_OLED
  if (!oledReady)
  {
    return;
  }
  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("=== WEATHER ===");
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

static void oledReading(const weather_sensor::Reading &r, bool sent)
{
#if WEATHER_OLED
  if (!r.valid)
  {
    oledShow("Sensor error", weather_sensor::typeName());
    return;
  }
  String t = "Temp: " + String(r.temperatureC, 1) + " C";
  String p = "Pres: " + String(r.pressureHPa, 1) + " hPa";
  String s = sent ? "Sent to Orchestrator" : "Send FAILED";
  oledShow(t, p, s);
#else
  (void)r;
  (void)sent;
#endif
}

// Bring up ESP-NOW and register the Orchestrator as a peer. Must be called after any
// portal session (which leaves WiFi in AP mode).
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

  uint8_t orchestratorMac[6];
  device_config::orchestratorMac(orchestratorMac);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, orchestratorMac, 6);
  peer.channel = MESH_CHANNEL;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  Serial.printf("ESP-NOW ready (channel %d)\n", MESH_CHANNEL);
  return true;
}

// Send one reading to the Orchestrator. Best-effort with a few retries; no fallback.
static bool sendReading(const weather_sensor::Reading &r)
{
  uint8_t orchestratorMac[6];
  device_config::orchestratorMac(orchestratorMac);

  WeatherMsg m = {};
  m.header.version = PROTOCOL_VERSION;
  m.header.deviceType = (uint8_t)DeviceType::WeatherNode;
  m.header.nodeId = device_config::nodeId();
  m.header.event = (uint8_t)WeatherEvent::Reading;
  m.header.seq = ++seqCounter;
  m.header.uptimeMs = millis();
  m.header.batteryPct = BATTERY_UNKNOWN;
  m.temperatureC = r.temperatureC;
  m.humidityPct = r.humidityPct; // NAN for BMP280
  m.pressureHPa = r.pressureHPa;

  for (int attempt = 0; attempt < 10; attempt++)
  {
    sendDone = false;
    sendOk = false;
    if (esp_now_send(orchestratorMac, (uint8_t *)&m, sizeof(m)) != ESP_OK)
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
      Serial.printf("Sent to orchestrator (seq %u)\n", (unsigned)m.header.seq);
      return true;
    }
  }
  Serial.println("Send to orchestrator failed (no fallback)");
  return false;
}

static void tick()
{
  // Blink blue while the portal is open (stealth-aware via rgb_led).
#if BUILTIN_LED_PIN >= 0
    if (millis() % 1500 > 750)
  {
    digitalWrite(BUILTIN_LED_PIN, HIGH);
  }
  else
  {
    digitalWrite(BUILTIN_LED_PIN, LOW);
  }
#else
  return;
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
  opts.deviceName = device_config::name(); // defaults to "Weather"
  opts.durationMs = device_config::portalWindowMs();
  opts.showMeshFields = true; // set the orchestrator MAC + mesh mode here
  opts.extraMenuLabel = "Settings";
  opts.extraMenuHref = "/settings";
  opts.registerExtraRoutes = weather_config::registerRoutes;
  opts.onTick = tick;
  opts.shouldExit = portalButtonClicked;

  web_portal::run(opts);
}

static void deepSleep()
{
  Serial.printf("Deep sleeping for %d s\n", WEATHER_SLEEP_SEC);
  Serial.flush();
  

#if WEATHER_OLED
  if (oledReady)
  {
    delay(10000);
    display.ssd1306_command(SSD1306_DISPLAYOFF); // Turns off the OLED glass display completely
  }
#endif

  esp_sleep_enable_timer_wakeup((uint64_t)WEATHER_SLEEP_SEC * 1000000ULL);
  esp_deep_sleep_start();
}

static void reportOnce()
{
  weather_sensor::Reading r = weather_sensor::read();
  if (r.valid)
  {
    Serial.printf("%s: %.1fC  %.1fhPa", weather_sensor::typeName(), r.temperatureC, r.pressureHPa);
    if (!isnan(r.humidityPct))
    {
      Serial.printf("  %.1f%%", r.humidityPct);
    }
    Serial.println();
  }
  else
  {
    Serial.println("Sensor read failed");
  }

  bool sent = sendReading(r);
  oledReading(r, sent);
}

void setup()
{
  Serial.begin(115200);
  delay(200);

  if (!weather_sensor::begin())
  {
    Serial.println("No supported sensor found (BMP280/BME280)");
  }

#if BUILTIN_LED_PIN >= 0
  pinMode(BUILTIN_LED_PIN, OUTPUT);
#endif

#if WEATHER_OLED
  Wire.begin(WEATHER_SDA, WEATHER_SCL); // no-op if the sensor already started I2C
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, WEATHER_OLED_ADDR);
  if (!oledReady)
  {
    Serial.println("OLED init failed");
  }
#endif

#if PORTAL_PIN >= 0
  pinMode(PORTAL_PIN, INPUT_PULLUP);
  bool portalHeld = digitalRead(PORTAL_PIN) == LOW;
#else
  bool portalHeld = false;
#endif

  // Open the setup portal on first run (no orchestrator provisioned) or on demand.
  if (portalHeld || !credentials::isConfigured())
  {
    oledShow("Setup mode", "AP: " + credentials::apSsid());
    Serial.printf("Opening setup portal (AP %s)\n", credentials::apSsid().c_str());
    openPortal();
    ESP.restart(); // re-evaluate cleanly with the new config
  }

  if (!espnowBegin())
  {
    deepSleep(); // nothing we can do; try again next wake
  }

  reportOnce();

#if WEATHER_SLEEP_SEC > 0
  deepSleep();
#endif
}

void loop()
{
#if WEATHER_SLEEP_SEC == 0
  reportOnce();
  delay(WEATHER_AWAKE_INTERVAL_MS);
#endif
}
