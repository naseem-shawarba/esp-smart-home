# ESP Smart Home — firmware

A small home security + telemetry system built from low-power **ESP32 / ESP32-C3** nodes.
ESP sensor nodes spend almost all their time in **deep sleep** and only wake on an event (a door
opening, a timer, a button). They report to a always-on **orchestrator** over **ESP-NOW**, and
the orchestrator bridges to the internet, sending **Telegram** alerts and **POSTing** sensor
readings to a home server. Every node is provisioned in the field through a captive **web
portal** (WiFi/Telegram/server URL, device identity) and updated **over-the-air** — no secrets
are baked into the binaries.

Each sensor node can also run **standalone** (connect to WiFi itself and send Telegram / POST
directly), chosen per-device in the portal — see the node READMEs.

> This repository is a **monorepo**: a shared library core in `shared/` consumed by several
> independently-buildable per-device firmwares in `firmware/`. This README documents the
> firmware collection; each node has its own README with build details.

---

## Architecture

```
   ┌────────────┐        ┌──────────────┐        ┌───────────────┐
   │  guard     │        │ simple-guard │        │   weather     │   ESP-NOW sensor nodes
   │ (ESP32)    │        │ (ESP32-C3)   │        │ (ESP32-C3)    │   (battery, deep sleep)
   └─────┬──────┘        └──────┬───────┘        └──────┬────────┘
         │  ESP-NOW (WeatherMsg / GuardMsg on MESH_CHANNEL)         │
         └───────────────┬──────┴─────────────────┬────────────────┘
                         ▼                         ▼
                  ┌───────────────────────────────────────┐
                  │            orchestrator (ESP32)         │  always-on gateway
                  │  receives ESP-NOW → Telegram / HTTP POST│
                  └───────────────┬─────────────────────────┘
                                  │ WiFi (on demand)
                        ┌─────────┴──────────┐
                        ▼                    ▼
                 Telegram chat        Home server / dashboard
```

---

## Nodes (`firmware/`)

| Node | Board | Role | README |
|------|-------|------|--------|
| **[guard](firmware/guard/)** | ESP32 DevKitC | Full door alarm: arm/disarm via 433 MHz remote, buzzer, RGB status, grace/cooldown phases, stealth mode. Reports over ESP-NOW (mesh) or Telegram (standalone). | [firmware/guard/README.md](firmware/guard/README.md) |
| **[simple-guard](firmware/simple-guard/)** | ESP32-C3 | Minimal battery door sensor: wakes on the reed switch, fires one `Triggered` event, sleeps. Mesh → orchestrator, or standalone → Telegram. Optional OLED. | [firmware/simple-guard/README.md](firmware/simple-guard/README.md) |
| **[weather](firmware/weather/)** | ESP32-C3 | Environment sensor: auto-detects BMP280 / BME280 / AHT10, reports on an interval. Mesh → orchestrator, or standalone → HTTP POST. Optional OLED. | [firmware/weather/README.md](firmware/weather/README.md) |
| **[orchestrator](firmware/orchestrator/)** | ESP32 DevKitC | Always-on gateway: receives ESP-NOW from the nodes, relays alarm events to **Telegram** and **POSTs** weather to a server. Goes online only when there's something to send. | [firmware/orchestrator/README.md](firmware/orchestrator/README.md) |

---

## Shared libraries (`shared/`)

Pulled into each firmware via `lib_extra_dirs = ../../shared`; a firmware only compiles the
shared libs it actually `#include`s.

| Library | Responsibility |
|---------|----------------|
| `espnow_protocol` | ESP-NOW message contract (`Header`, `GuardMsg`, `WeatherMsg`) + `MESH_CHANNEL`. Both nodes and orchestrator include it so the binary layout matches. |
| `credentials` | NVS `credentials` namespace: WiFi (PSK/Enterprise), Telegram, setup-AP. Composable checks `hasWifi()` / `hasTelegram()` + mode-aware `isConfigured()`. |
| `device_config` | Non-secret device config in NVS: name, node id, portal window, custom MAC, ESP-NOW mesh topology (mesh mode, orchestrator MAC), MAC-override apply. |
| `wifi_link` | Plain WiFi station join (PSK + WPA2-Enterprise + MAC override). `beginStation()` / `associate()` / `disconnect()` keep ESP-NOW alive; `connect()` for one-shot use. No Telegram/HTTP deps. |
| `connectivity` | Telegram messaging (`UniversalTelegramBot` over TLS); delegates the WiFi join to `wifi_link`. |
| `alarm_text` | Renders human-readable text for an alarm event (shared by guard + orchestrator). |
| `weather_config` | Shared "Settings" portal page + NVS store for the server POST URL. |
| `web_portal` | Temporary SoftAP + web server + captive-portal DNS; hosts the menu + OTA and composes the pages below via an `Options`/hooks struct. |
| `web_credentials` | Credentials form (`/credentials`) — WiFi/Telegram + optional mesh section (orchestrator MAC). |
| `web_device` | Device-settings page (`/device`) — name, node id, portal window, MAC override. |

---

## Building & uploading

Each node is its own [PlatformIO](https://platformio.org/) project (Espressif32 / Arduino,
arduino-esp32 2.0.17). Build from inside the node's folder:

```bash
cd firmware/<node>          # guard | simple-guard | weather | orchestrator
pio run                     # compile
pio run -t upload           # compile + flash over USB
pio device monitor          # serial monitor @ 115200 baud
```

> If `pio` isn't on your `PATH`, use the bundled binary, e.g.
> `~/.platformio/penv/bin/pio run`.

The compiled binary is at `firmware/<node>/.pio/build/<env>/firmware.bin`.  That's the file you
upload on the portal's **`/update`** page for OTA updates (no USB needed after the first flash).

Per-node **build flags** (I2C pins, sleep interval, OLED, default URLs, …) are documented in
each node's README.

---

## Provisioning (web portal)

Nothing is hard-coded. On first boot, (or when the active mode isn't configured, or on the
node's portal button ) a device brings up a **setup Access Point** and serves a captive config
menu:

| | |
|---|---|
| **SSID** | `Smart` |
| **Password** | `configure` |

Join it, set WiFi/Telegram, the mesh orchestrator MAC (mesh nodes) or the server URL
(standalone), device name/id, then flash firmware from `/update`. Credentials live in NVS,
never in git. See each node's README for its exact "configured" requirements.

> **ESP-NOW channel:** all nodes talk on `MESH_CHANNEL` (`shared/espnow_protocol`). The
> orchestrator listens on that channel and only hops to the router's channel briefly to send —
> so the router can be on any channel.
