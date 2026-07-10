# orchestrator — ESP-NOW gateway (ESP32)

The always-on hub. It listens for ESP-NOW messages from the sensor nodes and bridges them to
the internet: **alarm events → Telegram**, **weather readings → HTTP POST** to a home server.
To save power and keep the link reliable, it stays on the ESP-NOW channel by default and only
**briefly connects to WiFi when there's something to send**, then drops back.

## Overview

- **ESP-NOW receiver.** Decodes `GuardMsg` / `WeatherMsg` (see `shared/espnow_protocol`); each
  is queued out of the RX callback for processing in `loop()`.
- **On-demand uplink.** When a queue has data it `associate()`s to WiFi, drains the backlog
  (POST weather, relay alarm text to Telegram via `alarm_text`), then `disconnect()`s and
  restores `MESH_CHANNEL`. So the router can be on any channel.
- **Setup portal** for WiFi/Telegram, the server URL (`/settings`), device settings and OTA;
  re-openable by holding the BOOT button.

## Build & upload

```bash
cd firmware/orchestrator
pio run                # compile
pio run -t upload      # compile + flash over USB
pio device monitor     # serial @ 115200
```

Binary: `.pio/build/esp32dev/firmware.bin`. The serial log prints the **orchestrator MAC**
(enter it in each mesh node's portal) and the ESP-NOW channel at boot.

### Build flags (`platformio.ini`)

| Flag | Default | Purpose |
|------|---------|---------|
| `-D DEVICE_DEFAULT_NAME="<str>"` | `"Orchestrator"` | Default device name shown in the portal. |
| `-D DEFAULT_POST_URL="<url>"` | `""` (empty = weather POST disabled) | Default server ingest URL; also portal-editable on `/settings` (stored in NVS). |
| `-D BUILTIN_LED_PIN=<gpio>` | `2` | On-board LED blinked while the portal is open. |
| `-D PORTAL_PIN=<gpio>` | `0` (BOOT) | Button that re-opens the setup portal. |

## Configuration

Requires **WiFi + Telegram** to be considered configured (it relays alarm events over
Telegram); set the server POST URL on the **Settings** page to enable weather POSTing. The
mesh nodes must be provisioned with **this device's MAC** (printed at boot).

> The uplink window is a few seconds; while associated the orchestrator is briefly deaf to
> ESP-NOW on `MESH_CHANNEL`. Fine for weather; for alarm events consider it if you ever see
> drops.
