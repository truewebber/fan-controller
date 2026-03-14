#ifndef TACHOMETER_H
#define TACHOMETER_H

#include <Arduino.h>

class Tachometer {
public:
    struct Sample {
        unsigned int rpm = 0;
        unsigned int pulses = 0;
        unsigned long elapsedMs = 0;
    };

    void begin();
    bool update(unsigned long nowMs);
    const Sample& getLastSample() const;

    static void isrThunk();

private:
    static volatile unsigned int sPulseCount;
    static Tachometer* sInstance;

    unsigned long lastCalcMs_ = 0;
    Sample lastSample_{};

    void onPulse();
};

#endif  // TACHOMETER_H
