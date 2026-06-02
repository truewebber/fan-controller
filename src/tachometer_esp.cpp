#include "tachometer_esp.h"

volatile unsigned long TachometerEsp::sPulseCount_ = 0;

void IRAM_ATTR TachometerEsp::isrThunk() {
    ++sPulseCount_;
}

void TachometerEsp::begin(const RuntimeConfig& cfg) {
    pulsesPerRev_   = cfg.tachPulsesPerRev;
    calcIntervalMs_ = cfg.rpmCalcIntervalMs;

    pinMode(cfg.tachPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(cfg.tachPin), isrThunk, FALLING);
    lastCalcMs_ = millis();
}

bool TachometerEsp::update(unsigned long nowMs) {
    const unsigned long elapsed = nowMs - lastCalcMs_;
    if (elapsed < calcIntervalMs_) return false;

    noInterrupts();
    const unsigned long pulses = sPulseCount_;
    sPulseCount_ = 0;
    interrupts();

    lastSample_.pulses    = pulses;
    lastSample_.elapsedMs = elapsed;

    if (elapsed > 0 && pulsesPerRev_ > 0) {
        lastSample_.rpm = static_cast<unsigned int>(
            (pulses * 60000UL) / (static_cast<unsigned long>(pulsesPerRev_) * elapsed));
    } else {
        lastSample_.rpm = 0;
    }

    lastCalcMs_ = nowMs;
    return true;
}

const TachometerEsp::Sample& TachometerEsp::getLastSample() const {
    return lastSample_;
}
