# simple-guard — minimal door sensor (ESP32-C3)

A battery door sensor stripped to the essentials. It deep-sleeps until the reed switch reports
the door opened, delivers a single `Triggered` event, then sleeps again. No arming windows, no
buzzer. the notification logic lives on the [orchestrator](../orchestrator/) (mesh mode) or is
sent straight to Telegram (standalone mode).

## Overview

- **Wake on door open.** Deep-sleep GPIO wakeup on the reed switch (`DOOR_PIN`); on wake it
  sends one event and re-sleeps. Waits for the door to close first so it doesn't re-wake in a
  loop.
- **Two delivery modes** (portal mesh checkbox → `device_config::isMeshMode()`):
  - **Mesh** → `GuardMsg` (`AlarmEvent::Triggered`) to the orchestrator over ESP-NOW.
  - **Standalone** → connects to WiFi and sends the alarm text to **Telegram** directly
    (via `connectivity` + `alarm_text`).
- **Optional SSD1306 OLED** status + a portal-open LED.
- **Setup portal** (mesh/orchestrator MAC or WiFi/Telegram, device settings, OTA), opened on
  first boot / when the active mode isn't configured / on the BOOT button.

## Build & upload

```bash
cd firmware/simple-guard
pio run                # compile
pio run -t upload      # compile + flash over USB
pio device monitor     # serial @ 115200
```

Binary: `.pio/build/esp32-c3-devkitm-1/firmware.bin`.

### Build flags (`platformio.ini`)

| Flag | Default | Purpose |
|------|---------|---------|
| `-D DEVICE_DEFAULT_NODE_ID=<n>` | `1` | Default ESP-NOW node id (portal-editable). |
| `-D DEVICE_DEFAULT_NAME="<str>"` | `"Simple Guard"` | Default device name in the portal. |
| `-D DOOR_PIN=<gpio>` | `3` | Reed-switch input + deep-sleep wake. **Must be GPIO0–5 on the C3.** |
| `-D GUARD_SDA=<gpio>` | — | I2C SDA for the OLED. |
| `-D GUARD_SCL=<gpio>` | — | I2C SCL for the OLED. |
| `-D GUARD_OLED=<0\|1>` | `0` | Enable the SSD1306 OLED. |
| `-D GUARD_OLED_ADDR=<addr>` | `0x3C` | OLED I2C address. |
| `-D PORTAL_PIN=<gpio>` | `-1` (off) | BOOT button that opens the setup portal. |
| `-D BUILTIN_LED_PIN=<gpio>` | `-1` (off) | LED blinked while the portal is open. |
| `-D DOOR_CLOSE_TIMEOUT_MS=<ms>` | `30000` | Max wait for the door to close before sleeping. |

## Wiring

Reed switch between `DOOR_PIN` and GND; **door open reads HIGH** (reed + pull-up). An
**external pull resistor** to the closed level is recommended — the internal pull-up may not
hold through deep sleep on the C3.

## Configuration

- **Mesh mode:** needs the **orchestrator MAC** (portal → credentials/mesh section).
- **Standalone:** needs **WiFi + Telegram** (portal → credentials). The portal opens
  automatically when the active mode isn't configured.
