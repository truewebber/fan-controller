#pragma once

#include <Arduino.h>

static constexpr uint8_t kNumSensors = 10;

// Logical sensor slot indices — stable across config changes
namespace slot {
static constexpr uint8_t kRpi1Cpu  = 0;
static constexpr uint8_t kRpi1Nvme = 1;
static constexpr uint8_t kRpi2Cpu  = 2;
static constexpr uint8_t kRpi2Nvme = 3;
static constexpr uint8_t kRpi3Cpu  = 4;
static constexpr uint8_t kRpi3Nvme = 5;
static constexpr uint8_t kRpi4Cpu  = 6;
static constexpr uint8_t kRpi4Nvme = 7;
static constexpr uint8_t kIntake   = 8;
static constexpr uint8_t kExhaust  = 9;
}  // namespace slot

struct SensorConfig {
    uint8_t slot = 0;
    char name[20] = {};
    uint8_t address[8] = {};
};

struct RuntimeConfig {
    int version = 1;
    unsigned long baudRate = 115200;

    // Wi-Fi credentials
    char wifiSsid[33]     = {};
    char wifiPassword[64] = {};

    // GPIO pins — ESP-12F safe defaults (avoid 0,2,6-11,15,16)
    uint8_t fanPwmPin = 4;    // GPIO4
    uint8_t tachPin   = 5;    // GPIO5
    uint8_t oneWirePin = 14;  // GPIO14

    // Fan PWM
    uint8_t fanPwmMin       = 30;
    uint8_t fanPwmMax       = 255;
    unsigned int pwmFreqHz  = 25000;
    bool pwmActiveLow       = false;

    // Tachometer
    uint8_t tachPulsesPerRev       = 2;
    unsigned long rpmCalcIntervalMs = 1000;

    // DS18B20 sensors
    unsigned long tempReadIntervalMs = 3000;
    SensorConfig sensors[kNumSensors];

    // Auto-control thresholds (°C)
    float cpuTempMinC    = 40.0f;
    float cpuTempMaxC    = 75.0f;
    float nvmeTempMinC   = 40.0f;
    float nvmeTempMaxC   = 70.0f;
    float deltaTempMinC  = 5.0f;
    float deltaTempMaxC  = 15.0f;
    float fanCurveExp    = 2.5f;

    unsigned long statusPrintIntervalMs = 5000;
};

// Build a config struct seeded with project hardware defaults
RuntimeConfig defaultConfig();

// LittleFS persistence — returns false on first boot (file not found)
bool loadConfig(RuntimeConfig& cfg);
bool saveConfig(const RuntimeConfig& cfg);

// JSON conversion
bool parseConfig(const String& json, RuntimeConfig& cfg, String& error);
String serializeConfig(const RuntimeConfig& cfg);

// Validation — returns false and sets error on any constraint violation
bool validateConfig(const RuntimeConfig& cfg, String& error);
