#pragma once

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#include "runtime_config.h"

class TempSensorsEsp {
public:
    struct Sample {
        float maxCpuC   = 0.0f;
        float maxNvmeC  = 0.0f;
        float intakeC   = 0.0f;
        float exhaustC  = 0.0f;
        float deltaC    = 0.0f;
        bool hasCpu     = false;
        bool hasNvme    = false;
        bool hasIntake  = false;
        bool hasExhaust = false;
    };

    ~TempSensorsEsp();

    void begin(const RuntimeConfig& cfg);
    // Returns true when a new reading was taken.
    bool update(unsigned long nowMs);
    const Sample& getLastSample() const;
    // Individual per-slot readings, NAN if invalid.
    float getReading(uint8_t slot) const;
    bool isValid(uint8_t slot) const;
    int getFoundCount() const;

private:
    OneWire* oneWire_           = nullptr;
    DallasTemperature* sensors_ = nullptr;

    DeviceAddress addresses_[kNumSensors]{};
    float readings_[kNumSensors]{};
    bool valid_[kNumSensors]{};
    int foundCount_             = 0;
    unsigned long lastReadMs_   = 0;
    unsigned long intervalMs_   = 3000;
    Sample lastSample_{};

    void recomputeSample();
};
