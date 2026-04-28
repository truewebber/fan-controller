# ESP8266 (ESP-12F) Application Plan

This document describes the target architecture for migrating the fan controller from ATmega328 to ESP8266 (ESP-12F module). It assumes **no OTA**; firmware is flashed over USB. Runtime configuration is delivered over Wi‑Fi via HTTP. Human-edited **YAML** in the repository is converted to **JSON** before upload to the device (efficient on‑MCU storage and parsing).

---

## 1. Goals

- Preserve behavior: **auto fan curve** from CPU/NVMe/intake–exhaust delta temperatures, **manual PWM override**, **tachometer RPM**, **10× DS18B20** on one 1‑Wire bus.
- Add **Wi‑Fi** and a minimal **HTTP server**:
  - `GET /metrics` — Prometheus text exposition.
  - `GET /config` — current effective configuration (JSON).
  - `PUT /config` — replace configuration (JSON body), validate, persist, **restart** (recommended for consistent GPIO/timers).
- Configuration **no longer compile-time only**: defaults remain in firmware; live config stored on **LittleFS** as `config.json`.
- **YAML workflow**: developers edit `config.yaml` (or similar); a host-side tool (`yq`, Python, etc.) converts to JSON for `curl`/script upload.

---

## 2. Target Hardware

| Topic | Notes |
|--------|--------|
| Module | ESP-12F (ESP8266), 3.3 V I/O |
| Level shifting | Fan tach and any 5 V signals may need level shifters; DS18B20 wiring per your electrical design (parasitic vs external pull-up) |
| Pins | Map `kFanPwmPin`, `kTachPin`, `kOneWirePin` to ESP8266 GPIOs that support required functions (PWM, interrupt-capable input for tach, digital for 1‑Wire) — finalize in `config` schema and board docs |
| Current | ESP8266 GPIO drive capability differs from 5 V AVR; fan PWM switching stage must match 3.3 V logic or use a transistor/MOSFET stage as today |

---

## 3. High-Level Architecture

```
┌─────────────────────────────────────────────────────────┐
│  Boot: LittleFS mount → load config (or defaults)        │
│       Wi‑Fi connect → HTTP server + hardware init      │
└─────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
   TempSensors           Tachometer           FanController
   (1‑Wire / DS18B20)    (ISR + RPM)          (PWM output)
        │                     │                     │
        └─────────────────────┼─────────────────────┘
                              ▼
                     Control loop (auto / manual)
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
   HTTP handlers        Serial (optional)     Metrics cache
   /config /metrics     CLI parity             for /metrics
```

- **RuntimeConfig**: single in-RAM structure populated from JSON (defaults merged on load).
- **Persistence**: read/write `config.json` on LittleFS; optional `config.bak` after successful validation before overwrite.
- **Host tooling**: `tools/` script or documented `yq` one-liner: YAML → JSON; optional CI check that YAML parses and matches schema.

---

## 4. Configuration Model

### 4.1 Source of truth (repository)

- **`config/config.yaml`** (path negotiable): human-readable; documents pins, sensor ROM IDs, thresholds, intervals.
- Conversion to JSON is **not** performed on the MCU.

### 4.2 On-device format

- **`/config.json`** on LittleFS — UTF-8 JSON, same semantic fields as today’s `config.h` namespace `appcfg`.
- Include **`"version": 1`** (integer) for forward-compatible schema evolution.

### 4.3 Suggested JSON fields (mirror current `appcfg`)

| Area | Fields |
|------|--------|
| Serial | `baud_rate` |
| Pins | `fan_pwm_pin`, `tach_pin`, `onewire_pin` |
| Fan PWM | `fan_pwm_min`, `fan_pwm_max`, `pwm_frequency_hz` or `pwm_top` (platform-specific), `pwm_active_low` |
| Tach | `tach_pulses_per_revolution`, `rpm_calc_interval_ms` |
| Sensors | `total_sensors` (or fixed 10), `temp_read_interval_ms`, array `sensors[]` each with `slot`, `address_hex` (8 bytes, DS18B20 family `0x28`) |
| Auto tuning | `cpu_temp_min_c`, `cpu_temp_max_c`, `nvme_temp_min_c`, `nvme_temp_max_c`, `delta_temp_min_c`, `delta_temp_max_c`, `fan_curve_exponent` |
| Status | `status_print_interval_ms` (may map to metrics/logging cadence) |
| Logical slots | Keep indices consistent with current constants (`kSensorRpi1Cpu` … `kSensorExhaust`) for stable Prometheus labels |

### 4.4 Validation (before write or apply)

- Pin ranges and **no duplicate assignment** for PWM / tach / 1‑Wire.
- PWM min ≤ max; sensible temperature bounds; interval > 0.
- DS18B20: 8-byte addresses; family code `0x28`; optional CRC check against Dallas/Maxim algorithm.
- On `PUT` failure: return **400** with JSON `{"error":"..."}`; **do not** corrupt existing `config.json`.

### 4.5 Apply policy

- **Recommended:** successful `PUT` → persist → **`ESP.restart()`** so PWM timers, ISRs, and 1‑Wire bus align with new pins/settings.

---

## 5. HTTP API

| Method | Path | Request | Response |
|--------|------|---------|----------|
| GET | `/metrics` | — | `200`, `Content-Type: text/plain; version=0.0.4`, Prometheus text |
| GET | `/config` | — | `200`, `Content-Type: application/json`, current effective config (mask Wi‑Fi password if ever embedded) |
| PUT | `/config` | Body: full JSON | `200` + short JSON ack, or `400`/`500` with error object |
| * | *other* | — | `404` |

**Implementation notes:**

- Use **`ESP8266WebServer`** (Arduino core for ESP8266).
- **Do not** run blocking sensor reads inside handlers if they exceed normal cycle time; update a **metrics snapshot** in `loop()` and serve cached text on GET `/metrics`.
- Optional: shared secret via header `Authorization: Bearer <token>` for `PUT` and optionally `GET /config` on untrusted LANs.

---

## 6. Prometheus Metrics (Draft)

Use stable label sets; prefix with a project prefix e.g. `fan_controller_`.

**Fan / control**

- `fan_controller_pwm_duty` (gauge, 0–255 or normalized 0–1 — pick one and document).
- `fan_controller_auto_mode` (gauge 0/1).
- `fan_controller_tach_rpm` (gauge).
- `fan_controller_tach_pulses_total` (counter) — optional if you expose pulse count.

**Temperatures**

- Per-slot gauge: `fan_controller_temperature_celsius{slot="rpi1_cpu",...}` or numeric slot `sensor="0"` — prefer **named** labels matching YAML comments for readability.

**Device**

- `fan_controller_info` (info metric) — firmware version, optional config `version` field.
- `fan_controller_wifi_rssi_dbm` (gauge) — optional.
- Standard practice: avoid high-cardinality labels.

**Handler:** build text lines in a fixed buffer or `String` with care for heap fragmentation; prefer reusable static buffers for repeated scrape intervals.

---

## 7. Wi‑Fi

- **Station mode** primary: SSID/password from **compile-time secrets** (e.g. `secrets.h` gitignored) or from a **separate** small file on LittleFS (`wifi.json`) so `GET /config` does not automatically leak credentials — decide explicitly and document.
- Connection retries with backoff; optional `WiFi.status()` exposed as metric or serial log line.
- **No requirement** for AP provisioning in v1 unless desired for bring-up.

---

## 8. Main Loop Order (Recommended)

1. `WiFi` maintenance (reconnect if needed).
2. `server.handleClient()` (non-blocking).
3. `tachometer.update(now)`.
4. `tempSensors.update(now)` — if new sample and auto mode, recompute PWM.
5. `serialCommands.process(...)` if UART CLI remains.
6. Periodic **metrics snapshot** update (same data as printed today in STATUS).
7. Optional: periodic serial STATUS line (reduced frequency if metrics are primary).

---

## 9. Porting Tasks (AVR → ESP8266)

| Component | Action |
|-----------|--------|
| **platformio.ini** | Switch to `platform = espressif8266`, board `esp12e` or exact match for ESP-12F breakout; set `build_flags` / filesystem upload for LittleFS if needed |
| **PWM** | Replace Timer1 25 kHz ATmega code with ESP8266 PWM API (`analogWriteFrequency` where supported, or core-specific PWM) — re-verify frequency vs fan electronics |
| **Tach** | Reimplement interrupt / pulse counting on chosen GPIO; confirm pull-up and noise |
| **1‑Wire** | Keep OneWire + DallasTemperature; verify timing at 80 MHz and strong pull-up if parasitic |
| **SerialCommands** | Keep for debugging; baud from config |
| **config.h** | Shrink to defaults only + schema version; runtime values from `RuntimeConfig` |

---

## 10. Repository Layout (Suggested)

```
fan-controller/
├── ESP8266_APPLICATION_PLAN.md   # this file
├── platformio.ini
├── config/
│   └── config.yaml               # human-edited
├── tools/
│   └── yaml_to_json.sh           # or .py — YAML → JSON for PUT
├── data/                         # optional: initial LittleFS image for factory config
│   └── config.json
└── src/ / include/               # firmware sources
```

---

## 11. Build & Deploy Flow

1. Edit `config/config.yaml`.
2. Run converter → `config.json` artifact.
3. Flash firmware: `pio run -t upload`.
4. Upload filesystem or first-time `PUT /config` with the generated JSON.
5. Verify: `GET /metrics`, `GET /config`, Grafana/Prometheus scrape.

---

## 12. Security & Operational Notes

- HTTP is **plaintext**; restrict to management VLAN or add Bearer token on write endpoints.
- Document **recovery**: serial flash + wipe LittleFS or replace `config.json` if bad config prevents boot (watchdog + fallback defaults help).

---

## 13. Phased Delivery

| Phase | Deliverable |
|-------|-------------|
| **P1** | ESP8266 board profile; Wi‑Fi connect; LittleFS + load/save JSON defaults; HTTP GET `/config` |
| **P2** | PUT `/config` with validation + restart; YAML→JSON tool in repo |
| **P3** | Port temp/tach/PWM/auto logic; parity with AVR behavior |
| **P4** | GET `/metrics` + Prometheus scrape verified |
| **P5** | Hardening: auth header, optional `config.bak`, documentation |

---

## 14. Out of Scope (Current Decision)

- **OTA** (ArduinoOTA / HTTPUpdate) — not included unless requirements change.

---

## 15. Success Criteria

- Same control semantics as the AVR build under nominal conditions.
- Config change via **YAML → JSON → PUT** without recompile.
- Prometheus scrapes `/metrics` with all temperature slots, tach RPM, PWM, and auto mode.
- Documented pin map and electrical notes for ESP-12F.
