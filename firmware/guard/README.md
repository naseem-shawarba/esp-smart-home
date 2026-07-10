# guard — full door alarm (ESP32)

A low-power, battery-friendly security alarm on an **ESP32 DevKitC**. It sleeps almost all the
time and wakes only on an event: the door opening, a button on a 433 MHz remote, or the RTC
timer. When armed and the door opens, it sounds a buzzer and **notifies you** — either by
sending the event to the [orchestrator](../orchestrator/) over **ESP-NOW** (mesh mode), or by
connecting to WiFi and sending **Telegram** (standalone). It's armed/disarmed with a
wireless remote, configured over a **web portal**, and updated over-the-air.

## Overview

- **Event-driven around deep sleep.** Wakes via EXT1 (door/toggle/status/portal) or the RTC
  timer, acts on the cause, and sleeps again (~10 µA idle).
- **Arming with grace + cooldown phases.** Multi-minute "armed but waiting" periods are stored
  as a phase + deadline in RTC memory so the device deep-sleeps through them instead of staying
  awake.
- **Auto temporary-disable** after too many triggers (`maxAlarmTriggers`), then auto re-arm.
- **Mode-aware reporting** (`notifier`): mesh (ESP-NOW → orchestrator) with optional Telegram
  fallback, or fully standalone Telegram.
- **Stealth mode**: LED dark + buzzer silent.

## Hardware & wiring

| ESP32 GPIO | Connected to | Notes |
|-----------|--------------|-------|
| **14** | MC-38 door reed switch (NC) | wake input, 10 kΩ pull-down; **HIGH = open** |
| **27** | RF receiver → toggle (arm/disarm) | wake input, 10 kΩ pull-down |
| **4**  | RF receiver → status | wake input, 10 kΩ pull-down |
| **13** | RF receiver → portal (config/OTA) | wake input, 10 kΩ pull-down |
| **19 / 22 / 23** | RGB LED R / G / B (via 220 Ω) | PWM status LED (common-cathode) |
| **21** | Passive piezo buzzer (+) | `tone()`, 2 kHz |

Wake uses **EXT1 `ESP_EXT1_WAKEUP_ANY_HIGH`**: every wake input idles LOW (pull-downs) and is
driven HIGH on its event. GPIO 14/27/4/13 are RTC-capable and non-strapping. Full wiring
diagrams and the RGB colour legend are in the pin map in `include/config.h`.

## Build & upload

```bash
cd firmware/guard
pio run                # compile
pio run -t upload      # compile + flash over USB
pio device monitor     # serial @ 115200
```

Binary: `.pio/build/esp32dev/firmware.bin` (upload on the portal `/update` page for OTA).

### Build flags (`platformio.ini`)

| Flag | Default | Purpose |
|------|---------|---------|
| `-I include` | — | Adds `include/` to the header path so libs find `config.h`. |
| `-D DEVICE_DEFAULT_NODE_ID=<n>` | `1` | Default ESP-NOW node id (portal-editable, stored in NVS). |
| `-D DEVICE_DEFAULT_NAME="<str>"` | `"Door"` | Default device name shown in the portal (portal-editable). |

> Pins, timings, buzzer settings and `maxAlarmTriggers` are **not** build flags — they live in
> `include/config.h`. Runtime settings (stealth, windows, delays) are portal-editable and stored
> in NVS.

## Configuration (web portal)

On first boot (or on the portal button) it starts the setup AP `Smart` / `configure` at
`http://10.10.10.1/`. Pages: `/credentials` (WiFi + Telegram + optional mesh/orchestrator MAC),
`/settings` (stealth + timings), `/device` (name, node id, portal window, MAC), `/update` (OTA).
Password/token fields are write-only (blank = keep current).

Runtime settings (portal-editable, NVS):

| Setting | Default | Description |
|---------|---------|-------------|
| Stealth mode | off | LED dark + buzzer silent. |
| Deactivation window | 8 s | Time to disarm after the door opens before it sounds. |
| LED blink interval | 500 ms | Blink rate during the disarm window. |
| Post-activation delay | 30 min | Grace period after arming before reporting door state. |
| Post-trigger delay | 20 min | Cooldown after the alarm sounds. |
| Temporary-disable period | 24 h | How long it stays auto-disabled after too many triggers. |
| Portal window | 5 min | How long the portal stays open. |

## Firmware modules (`lib/`)

| Module | Responsibility |
|--------|----------------|
| `alarm` | Alarm state, arm/disarm, status display, timed **phase** machine (grace/cooldown/temp-disable) in RTC memory |
| `deep_sleep` | Wake-cause dispatch + phase-aware deep-sleep configuration |
| `notifier` | Mode-aware reporting: mesh (ESP-NOW) vs standalone (Telegram), with fallback |
| `mesh` | ESP-NOW sender to the orchestrator |
| `rgb_led` | RGB status LED (stealth gating, sleep-safe off) |
| `buzzer` | Piezo tones (stealth-aware) |
| `settings` | Runtime settings persisted in NVS |
| `web_config` | The `/settings` form routes |

Shared libs (`credentials`, `device_config`, `connectivity`, `alarm_text`, `wifi_link`,
`web_portal`, `web_credentials`, `web_device`) come from `../../shared`.
