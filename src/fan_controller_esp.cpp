#include "fan_controller_esp.h"

void FanControllerEsp::begin(const RuntimeConfig& cfg) {
    pin_       = cfg.fanPwmPin;
    pwmMin_    = cfg.fanPwmMin;
    pwmMax_    = cfg.fanPwmMax;
    activeLow_ = cfg.pwmActiveLow;

    pinMode(pin_, OUTPUT);
    // Keep PWM resolution at 0-255 (8-bit) for compatibility with config values.
    analogWriteRange(255);
    analogWriteFreq(cfg.pwmFreqHz);

    setPwm(pwmMin_);
}

void FanControllerEsp::setPwm(uint8_t pwm) {
    pwm = static_cast<uint8_t>(constrain(pwm, pwmMin_, pwmMax_));
    current_ = pwm;
    const uint8_t out = activeLow_ ? (255 - pwm) : pwm;
    analogWrite(pin_, out);
}

uint8_t FanControllerEsp::getPwm() const {
    return current_;
}
