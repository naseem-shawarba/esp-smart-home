#include "weather_sensor.h"

#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>

// I2C pins — default to ESP32
#ifndef WEATHER_SDA
#define WEATHER_SDA 21
#endif
#ifndef WEATHER_SCL
#define WEATHER_SCL 22
#endif

namespace weather_sensor
{
  // Bosch chip-ID register and the IDs it returns.
  static const uint8_t REG_CHIP_ID = 0xD0;
  static const uint8_t CHIP_ID_BME280 = 0x60;
  // BMP280 reports 0x58, some clones report 0x56/0x57.

  RTC_DATA_ATTR static uint8_t s_type = 0; // Type
  RTC_DATA_ATTR static uint8_t s_addr = 0; // I2C address, 0 = unknown

  // Only the detected sensor is actually begin()ed, the other stays idle.
  static Adafruit_BMP280 bmp;
  static Adafruit_BME280 bme;

  static uint8_t readChipId(uint8_t addr)
  {
    Wire.beginTransmission(addr);
    Wire.write(REG_CHIP_ID);
    if (Wire.endTransmission() != 0)
    {
      return 0x00; // nothing acked at this address
    }
    if (Wire.requestFrom(addr, (uint8_t)1) != 1)
    {
      return 0x00;
    }
    return Wire.read();
  }

  // Probe both common addresses; classify by chip ID. Stores into s_type/s_addr.
  static void probe()
  {
    s_type = (uint8_t)Type::Unknown;
    s_addr = 0;
    const uint8_t addrs[] = {0x76, 0x77};
    for (uint8_t i = 0; i < 2; i++)
    {
      uint8_t id = readChipId(addrs[i]);
      if (id == CHIP_ID_BME280)
      {
        s_type = (uint8_t)Type::BME280;
        s_addr = addrs[i];
        return;
      }
      if (id == 0x56 || id == 0x57 || id == 0x58)
      {
        s_type = (uint8_t)Type::BMP280;
        s_addr = addrs[i];
        return;
      }
    }
  }

  static bool beginDetected()
  {
    switch ((Type)s_type)
    {
    case Type::BMP280:
      return bmp.begin(s_addr);
    case Type::BME280:
      return bme.begin(s_addr);
    default:
      return false;
    }
  }

  bool begin()
  {
    Wire.begin(WEATHER_SDA, WEATHER_SCL);

    // Trust the RTC cache first; fall back to a fresh probe if it's empty or wrong.
    if ((Type)s_type != Type::Unknown && s_addr != 0 && beginDetected())
    {
      Serial.printf("Weather sensor: %s @ 0x%02X (cached)\n", typeName(), s_addr);
      return true;
    }

    probe();
    if ((Type)s_type == Type::Unknown || !beginDetected())
    {
      Serial.println("Weather sensor: none detected");
      s_type = (uint8_t)Type::Unknown;
      s_addr = 0;
      return false;
    }
    Serial.printf("Weather sensor: %s @ 0x%02X (detected)\n", typeName(), s_addr);
    return true;
  }

  Reading read()
  {
    Reading r = {};
    r.humidityPct = NAN;
    switch ((Type)s_type)
    {
    case Type::BMP280:
      r.temperatureC = bmp.readTemperature();
      r.pressureHPa = bmp.readPressure() / 100.0f;
      r.valid = !isnan(r.temperatureC);
      break;
    case Type::BME280:
      r.temperatureC = bme.readTemperature();
      r.pressureHPa = bme.readPressure() / 100.0f;
      r.humidityPct = bme.readHumidity();
      r.valid = !isnan(r.temperatureC);
      break;
    default:
      r.valid = false;
      break;
    }
    return r;
  }

  Type type()
  {
    return (Type)s_type;
  }

  const char *typeName()
  {
    switch ((Type)s_type)
    {
    case Type::BMP280:
      return "BMP280";
    case Type::BME280:
      return "BME280";
    default:
      return "unknown";
    }
  }
}
