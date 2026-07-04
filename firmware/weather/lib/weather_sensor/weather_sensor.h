#pragma once

#include <Arduino.h>

// Auto-detecting environmental sensor for the weather node.
//
// Supports the pin-compatible Bosch BMP280 (temp + pressure) and BME280
// (temp + pressure + humidity), which share I2C addresses and differ only by a
// chip-ID register. The detected type is cached in RTC memory so it survives
// deep sleep and only the first cold boot pays the probe cost.
//
// I2C pins / addresses come from build flags (see defaults below) so the same
// firmware runs on different boards without code changes.
namespace weather_sensor
{
  enum class Type : uint8_t
  {
    Unknown = 0,
    BMP280 = 1, // temperature + pressure (no humidity)
    BME280 = 2, // temperature + pressure + humidity
  };

  struct Reading
  {
    bool valid;         // false if the sensor could not be read
    float temperatureC;
    float humidityPct;  // NAN when the sensor has no humidity channel (BMP280)
    float pressureHPa;
  };

  // Start I2C and initialise whichever sensor is present. Uses the RTC-cached
  // type when available, otherwise probes the bus and caches the result in RTC_DATA_ATTR memory.
  // Returns false if no supported sensor responds.
  bool begin();

  Reading read();

  Type type();
  const char *typeName(); // "BMP280" / "BME280" / "unknown"
}
