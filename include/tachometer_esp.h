#pragma once

#include <Arduino.h>

#include "runtime_config.h"

class TachometerEsp {
public:
    struct Sample {
        unsigned int rpm      = 0;
        unsigned long pulses  = 0;
        unsigned long elapsedMs = 0;
    };

    void begin(const RuntimeConfig& cfg);
    bool update(unsigned long nowMs);
    const Sample& getLastSample() const;

    static void IRAM_ATTR isrThunk();

private:
    static volatile unsigned long sPulseCount_;

    uint8_t pulsesPerRev_        = 2;
    unsigned long calcIntervalMs_ = 1000;
    unsigned long lastCalcMs_    = 0;
    Sample lastSample_{};
};
