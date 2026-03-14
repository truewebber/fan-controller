#ifndef TEMP_SENSORS_H
#define TEMP_SENSORS_H

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>

class TempSensors {
public:
    struct Sample {
        float maxCpuC = 0.0f;
        float maxNvmeC = 0.0f;
        float intakeC = 0.0f;
        float exhaustC = 0.0f;
        float deltaC = 0.0f;
        bool hasCpu = false;
        bool hasNvme = false;
        bool hasIntake = false;
        bool hasExhaust = false;
    };

    TempSensors();
    void begin();
    bool update(unsigned long nowMs);  // true when new sample was read
    const Sample& getLastSample() const;
    int getFoundCount() const;

private:
    OneWire oneWire_;
    DallasTemperature sensors_;
    DeviceAddress addresses_[10]{};
    float readings_[10]{};
    bool valid_[10]{};
    int foundCount_ = 0;
    unsigned long lastReadMs_ = 0;
    Sample lastSample_{};

    void recomputeSample();
};

#endif  // TEMP_SENSORS_H
