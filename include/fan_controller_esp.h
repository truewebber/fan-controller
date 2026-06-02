#pragma once

#include <Arduino.h>

#include "runtime_config.h"

class FanControllerEsp {
public:
    void begin(const RuntimeConfig& cfg);
    void setPwm(uint8_t pwm);
    uint8_t getPwm() const;

private:
    uint8_t pin_    = 4;
    uint8_t pwmMin_ = 30;
    uint8_t pwmMax_ = 255;
    bool activeLow_ = false;
    uint8_t current_ = 0;
};
