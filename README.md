# RPi Fan Controller (Arduino Pro Mini)

Arduino firmware for controlling a 4-pin PWM fan using:

- tachometer feedback (`TACH`)
- 10x DS18B20 temperature sensors on one OneWire bus
- automatic fan curve logic based on:
  - max CPU temperature
  - max NVMe temperature
  - board airflow delta (`exhaust - intake`)

## Hardware

- Arduino Pro Mini 5V / 16MHz (ATmega328P)
- 4-pin PWM fan (tested with Noctua NF-A12x25 5V PWM)
- 10x GX18B20S (TO-92S form factor, DS18B20-compatible 1-Wire sensors)
- optional: 10k pull-up on OneWire line (if not already present)

## Pinout

- `D9`  -> fan PWM (Timer1 / OC1A, 25kHz)
- `D3`  -> fan tachometer input
- `D2`  -> OneWire bus for DS18B20
- `GND` -> common ground (Arduino + fan + sensor bus)

## Sensor Map (10 total)

By bus index:

- `0` RPI1 CPU
- `1` RPI1 NVME
- `2` RPI2 CPU
- `3` RPI2 NVME
- `4` RPI3 CPU
- `5` RPI3 NVME
- `6` RPI4 CPU
- `7` RPI4 NVME
- `8` Board intake
- `9` Board exhaust

At boot firmware prints discovered ROM addresses with indices.

## Architecture

- `src/pwm_output.cpp`: low-level Timer1 PWM (25kHz)
- `src/tachometer.cpp`: interrupt pulse counting and RPM calculation
- `src/temp_sensors.cpp`: OneWire scan and DS18B20 sampling
- `src/fan_controller.cpp`: fan PWM clamp and output
- `src/serial_commands.cpp`: command parser (`STATUS`, `PWM`, `AUTO`)
- `src/main.cpp`: wiring/orchestration loop

## Fan Control Logic

Every temperature update:

1. Read all DS18B20 sensors
2. Compute:
   - `maxCpuC` from CPU sensors
   - `maxNvmeC` from NVMe sensors
   - `deltaC = max(0, exhaust - intake)`
3. Convert each signal to demand `[0..1]` using thresholds + curve exponent
4. Take maximum demand
5. Map demand to PWM range `[kFanPwmMin..kFanPwmMax]`

Manual PWM command disables auto mode until `AUTO ON`.

## Serial Commands

Use Serial Monitor at `9600` baud:

- `STATUS`
- `HELP`
- `AUTO ON`
- `AUTO OFF`
- `PWM:<0..255>` (example: `PWM:140`)

Example status line:

`STATUS pwm=140 auto=ON pulses=57 elapsedMs=1002 rpm=1706 cpuMax=52.0 nvmeMax=48.5 delta=3.1`

## Build and Upload

```bash
pio run
pio run -t upload
pio device monitor
```

## Config Tuning

All runtime constants are in `include/config.h`:

- PWM polarity (`kPwmActiveLow`)
- temperature thresholds
- curve exponent
- sensor read period
- status print period

## Notes

- Project is intentionally UART-free now (no RPi soft-serial protocol).
- For reliable tach signal, keep `TACH` physically close to `GND` wiring.
- If you use an external inverting PWM stage (NPN open-collector), set `kPwmActiveLow = true`.
