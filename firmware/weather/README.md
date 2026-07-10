# weather — environment sensor (ESP32-C3)

A battery weather node. It auto-detects the attached sensor, takes a reading, delivers it,
optionally shows it on an OLED, then deep-sleeps for the reporting interval.

## Overview

- **Auto-detecting sensor** (`weather_sensor`, RTC-cached type): **BMP280** (temp + pressure),
  **BME280** (+ humidity), or **AHT10** (temp + humidity). Probes I2C on boot; caches the type
  in RTC so later wakes skip the probe.
- **Two delivery modes** (portal mesh checkbox → `device_config::isMeshMode()`):
  - **Mesh** → sends a `WeatherMsg` to the [orchestrator](../orchestrator/) over ESP-NOW.
  - **Standalone** → connects to WiFi and **POSTs** `{"temperature",…,"pressure"}` to the
    server URL itself.
- **Change-gated reporting** (`USE_APPROX_MODE`): rounds readings and only transmits when the
  value actually changed vs the RTC-cached last value — saves radio/battery when stable.
- **Optional SSD1306 OLED** and a portal-open LED.
- **Deep sleep** for `WEATHER_SLEEP_SEC` (set `0` to stay awake in a loop, e.g. a bench display).

## Build & upload

```bash
cd firmware/weather
pio run                # compile
pio run -t upload      # compile + flash over USB
pio device monitor     # serial @ 115200
```

Binary: `.pio/build/esp32-c3-devkitm-1/firmware.bin`.

### Build flags (`platformio.ini`)

| Flag | Default | Purpose |
|------|---------|---------|
| `-D DEVICE_DEFAULT_NODE_ID=<n>` | `10` | Default ESP-NOW node id (portal-editable). |
| `-D DEVICE_DEFAULT_NAME="<str>"` | `"Weather"` | Default device name in the portal. |
| `-D WEATHER_SDA=<gpio>` | `21` | I2C SDA for the sensor + OLED. |
| `-D WEATHER_SCL=<gpio>` | `22` | I2C SCL. |
| `-D WEATHER_SLEEP_SEC=<sec>` | `600` | Deep-sleep interval between readings. `0` = never sleep (loop). |
| `-D WEATHER_AWAKE_INTERVAL_MS=<ms>` | `10000` | Loop interval when `WEATHER_SLEEP_SEC=0`. |
| `-D DEFAULT_POST_URL="<url>"` | `""` | Default standalone POST URL (portal-editable on `/settings`). |
| `-D WEATHER_OLED=<0\|1>` | `0` | Enable the SSD1306 OLED. |
| `-D WEATHER_OLED_ADDR=<addr>` | `0x3C` | OLED I2C address. |
| `-D WEATHER_OLED_WIDTH` / `_HEIGHT` | `128` / `64` | OLED resolution. |
| `-D PORTAL_PIN=<gpio>` | `-1` (off) | Button that opens the setup portal at boot. |
| `-D BUILTIN_LED_PIN=<gpio>` | `-1` (off) | LED blinked while the portal is open. |
| `-D USE_APPROX_MODE=<0\|1>` | `0` | Round readings + only transmit on change. |

## Configuration

- **Mesh mode:** needs the **orchestrator MAC** (portal → credentials/mesh section).
- **Standalone:** needs a **POST URL** (`/settings`) **and WiFi** (portal → credentials). The
  portal opens automatically when the active mode isn't configured.
