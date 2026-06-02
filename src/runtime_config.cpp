#include "runtime_config.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <string.h>

static constexpr const char* kConfigPath = "/config.json";

// Default DS18B20 ROM addresses matching the installed hardware.
static const uint8_t kDefaultAddresses[kNumSensors][8] = {
    {0x28, 0xC9, 0x1D, 0x7B, 0x02, 0x25, 0x0E, 0xD5},  // 0 rpi1_cpu
    {0x28, 0x7A, 0xC0, 0x7C, 0x02, 0x25, 0x0E, 0xBF},  // 1 rpi1_nvme
    {0x28, 0xCD, 0xE1, 0x6A, 0x02, 0x25, 0x0E, 0x39},  // 2 rpi2_cpu
    {0x28, 0x2E, 0x56, 0x68, 0x02, 0x25, 0x0E, 0x50},  // 3 rpi2_nvme
    {0x28, 0x2B, 0xFA, 0x5B, 0x02, 0x25, 0x0E, 0x69},  // 4 rpi3_cpu
    {0x28, 0xC1, 0xE3, 0x4B, 0x02, 0x25, 0x0E, 0x38},  // 5 rpi3_nvme
    {0x28, 0x1D, 0xD5, 0x76, 0x02, 0x25, 0x0E, 0x4C},  // 6 rpi4_cpu
    {0x28, 0x45, 0x5B, 0x50, 0x02, 0x25, 0x0E, 0xD3},  // 7 rpi4_nvme
    {0x28, 0x1D, 0xF8, 0x68, 0x02, 0x25, 0x0E, 0x75},  // 8 intake
    {0x28, 0x5D, 0x20, 0x7A, 0x02, 0x25, 0x0E, 0x5F},  // 9 exhaust
};

static const char* kDefaultNames[kNumSensors] = {
    "rpi1_cpu", "rpi1_nvme",
    "rpi2_cpu", "rpi2_nvme",
    "rpi3_cpu", "rpi3_nvme",
    "rpi4_cpu", "rpi4_nvme",
    "intake",   "exhaust",
};

// ---------------------------------------------------------------------------

RuntimeConfig defaultConfig() {
    RuntimeConfig cfg;
    for (uint8_t i = 0; i < kNumSensors; ++i) {
        cfg.sensors[i].slot = i;
        strncpy(cfg.sensors[i].name, kDefaultNames[i], sizeof(cfg.sensors[i].name) - 1);
        memcpy(cfg.sensors[i].address, kDefaultAddresses[i], 8);
    }
    return cfg;
}

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------

static bool hexToAddress(const char* hex, uint8_t* out) {
    if (!hex || strlen(hex) != 16) return false;
    for (int i = 0; i < 8; ++i) {
        char buf[3] = { hex[i * 2], hex[i * 2 + 1], '\0' };
        char* end;
        unsigned long v = strtoul(buf, &end, 16);
        if (end != buf + 2) return false;
        out[i] = static_cast<uint8_t>(v);
    }
    return true;
}

static String addressToHex(const uint8_t* addr) {
    String s;
    s.reserve(16);
    for (int i = 0; i < 8; ++i) {
        if (addr[i] < 0x10) s += '0';
        s += String(addr[i], HEX);
    }
    s.toUpperCase();
    return s;
}

bool validateConfig(const RuntimeConfig& cfg, String& error) {
    // Pin uniqueness
    if (cfg.fanPwmPin == cfg.tachPin || cfg.fanPwmPin == cfg.oneWirePin ||
        cfg.tachPin == cfg.oneWirePin) {
        error = "Duplicate pin assignment";
        return false;
    }
    // ESP8266 valid GPIO range (skip 6-11: flash SPI)
    auto validPin = [](uint8_t p) { return p <= 16 && !(p >= 6 && p <= 11); };
    if (!validPin(cfg.fanPwmPin) || !validPin(cfg.tachPin) || !validPin(cfg.oneWirePin)) {
        error = "Invalid pin (valid: 0-5, 12-16)";
        return false;
    }
    if (cfg.fanPwmMin > cfg.fanPwmMax) {
        error = "fan_pwm_min > fan_pwm_max";
        return false;
    }
    if (cfg.cpuTempMinC >= cfg.cpuTempMaxC) { error = "cpu_temp_min_c >= max"; return false; }
    if (cfg.nvmeTempMinC >= cfg.nvmeTempMaxC) { error = "nvme_temp_min_c >= max"; return false; }
    if (cfg.deltaTempMinC >= cfg.deltaTempMaxC) { error = "delta_temp_min_c >= max"; return false; }
    if (cfg.rpmCalcIntervalMs == 0 || cfg.tempReadIntervalMs == 0 ||
        cfg.statusPrintIntervalMs == 0) {
        error = "Interval must be > 0";
        return false;
    }
    for (uint8_t i = 0; i < kNumSensors; ++i) {
        if (cfg.sensors[i].address[0] != 0x28) {
            error = String("Sensor ") + i + ": invalid family code (expected 0x28)";
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------

bool parseConfig(const String& json, RuntimeConfig& cfg, String& error) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        error = String("JSON: ") + err.c_str();
        return false;
    }

#define LOAD(field, key) if (!doc[key].isNull()) cfg.field = doc[key]
#define LOAD_STR(buf, key) \
    if (!doc[key].isNull()) strncpy(buf, doc[key] | "", sizeof(buf) - 1)

    LOAD(version,              "version");
    LOAD(baudRate,             "baud_rate");
    LOAD_STR(cfg.wifiSsid,     "wifi_ssid");
    LOAD_STR(cfg.wifiPassword, "wifi_password");
    LOAD(fanPwmPin,            "fan_pwm_pin");
    LOAD(tachPin,              "tach_pin");
    LOAD(oneWirePin,           "onewire_pin");
    LOAD(fanPwmMin,            "fan_pwm_min");
    LOAD(fanPwmMax,            "fan_pwm_max");
    LOAD(pwmFreqHz,            "pwm_frequency_hz");
    LOAD(pwmActiveLow,         "pwm_active_low");
    LOAD(tachPulsesPerRev,     "tach_pulses_per_revolution");
    LOAD(rpmCalcIntervalMs,    "rpm_calc_interval_ms");
    LOAD(tempReadIntervalMs,   "temp_read_interval_ms");
    LOAD(cpuTempMinC,          "cpu_temp_min_c");
    LOAD(cpuTempMaxC,          "cpu_temp_max_c");
    LOAD(nvmeTempMinC,         "nvme_temp_min_c");
    LOAD(nvmeTempMaxC,         "nvme_temp_max_c");
    LOAD(deltaTempMinC,        "delta_temp_min_c");
    LOAD(deltaTempMaxC,        "delta_temp_max_c");
    LOAD(fanCurveExp,          "fan_curve_exponent");
    LOAD(statusPrintIntervalMs,"status_print_interval_ms");
#undef LOAD
#undef LOAD_STR

    if (doc["sensors"].is<JsonArray>()) {
        for (JsonObject obj : doc["sensors"].as<JsonArray>()) {
            int s = obj["slot"] | -1;
            if (s < 0 || s >= kNumSensors) continue;
            cfg.sensors[s].slot = static_cast<uint8_t>(s);
            const char* name = obj["name"] | "";
            strncpy(cfg.sensors[s].name, name, sizeof(cfg.sensors[s].name) - 1);
            const char* hex = obj["address"] | "";
            hexToAddress(hex, cfg.sensors[s].address);
        }
    }

    return validateConfig(cfg, error);
}

String serializeConfig(const RuntimeConfig& cfg) {
    JsonDocument doc;
    doc["version"]                    = cfg.version;
    doc["baud_rate"]                  = cfg.baudRate;
    doc["wifi_ssid"]                  = cfg.wifiSsid;
    doc["wifi_password"]              = cfg.wifiPassword;
    doc["fan_pwm_pin"]                = cfg.fanPwmPin;
    doc["tach_pin"]                   = cfg.tachPin;
    doc["onewire_pin"]                = cfg.oneWirePin;
    doc["fan_pwm_min"]                = cfg.fanPwmMin;
    doc["fan_pwm_max"]                = cfg.fanPwmMax;
    doc["pwm_frequency_hz"]           = cfg.pwmFreqHz;
    doc["pwm_active_low"]             = cfg.pwmActiveLow;
    doc["tach_pulses_per_revolution"] = cfg.tachPulsesPerRev;
    doc["rpm_calc_interval_ms"]       = cfg.rpmCalcIntervalMs;
    doc["temp_read_interval_ms"]      = cfg.tempReadIntervalMs;
    doc["cpu_temp_min_c"]             = cfg.cpuTempMinC;
    doc["cpu_temp_max_c"]             = cfg.cpuTempMaxC;
    doc["nvme_temp_min_c"]            = cfg.nvmeTempMinC;
    doc["nvme_temp_max_c"]            = cfg.nvmeTempMaxC;
    doc["delta_temp_min_c"]           = cfg.deltaTempMinC;
    doc["delta_temp_max_c"]           = cfg.deltaTempMaxC;
    doc["fan_curve_exponent"]         = cfg.fanCurveExp;
    doc["status_print_interval_ms"]   = cfg.statusPrintIntervalMs;

    JsonArray arr = doc["sensors"].to<JsonArray>();
    for (uint8_t i = 0; i < kNumSensors; ++i) {
        JsonObject o = arr.add<JsonObject>();
        o["slot"]    = cfg.sensors[i].slot;
        o["name"]    = cfg.sensors[i].name;
        o["address"] = addressToHex(cfg.sensors[i].address);
    }

    String out;
    serializeJsonPretty(doc, out);
    return out;
}

// ---------------------------------------------------------------------------

bool loadConfig(RuntimeConfig& cfg) {
    if (!LittleFS.exists(kConfigPath)) {
        Serial.println("[cfg] config.json not found, using defaults");
        return false;
    }
    File f = LittleFS.open(kConfigPath, "r");
    if (!f) {
        Serial.println("[cfg] open failed");
        return false;
    }
    const String json = f.readString();
    f.close();

    String error;
    if (!parseConfig(json, cfg, error)) {
        Serial.print("[cfg] parse error: ");
        Serial.println(error);
        return false;
    }
    Serial.println("[cfg] loaded from LittleFS");
    return true;
}

bool saveConfig(const RuntimeConfig& cfg) {
    File f = LittleFS.open(kConfigPath, "w");
    if (!f) {
        Serial.println("[cfg] save: open failed");
        return false;
    }
    f.print(serializeConfig(cfg));
    f.close();
    Serial.println("[cfg] saved to LittleFS");
    return true;
}
