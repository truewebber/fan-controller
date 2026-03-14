#include "tachometer.h"

#include "config.h"

volatile unsigned int Tachometer::sPulseCount = 0;
Tachometer* Tachometer::sInstance = nullptr;

void Tachometer::begin() {
    sInstance = this;
    pinMode(appcfg::kTachPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(appcfg::kTachPin), Tachometer::isrThunk, FALLING);
    lastCalcMs_ = millis();
}

bool Tachometer::update(unsigned long nowMs) {
    const unsigned long elapsed = nowMs - lastCalcMs_;
    if (elapsed < appcfg::kRpmCalcIntervalMs) {
        return false;
    }

    noInterrupts();
    const unsigned int pulses = sPulseCount;
    sPulseCount = 0;
    interrupts();

    lastSample_.pulses = pulses;
    lastSample_.elapsedMs = elapsed;

    // RPM = pulses * 60000 / (pulses_per_rev * elapsed_ms)
    if (elapsed > 0) {
        lastSample_.rpm = static_cast<unsigned int>(
            (static_cast<unsigned long>(pulses) * 60000UL) /
            (static_cast<unsigned long>(appcfg::kTachPulsesPerRevolution) * elapsed));
    } else {
        lastSample_.rpm = 0;
    }

    lastCalcMs_ = nowMs;
    return true;
}

const Tachometer::Sample& Tachometer::getLastSample() const {
    return lastSample_;
}

void Tachometer::isrThunk() {
    if (sInstance != nullptr) {
        sInstance->onPulse();
    }
}

void Tachometer::onPulse() {
    sPulseCount++;
}
