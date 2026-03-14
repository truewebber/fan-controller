#include "fan_controller.h"

#include "config.h"

void FanController::begin() {
    pwmOut_.begin();
    setPwm(appcfg::kFanPwmMin);
}

void FanController::setPwm(uint8_t pwm) {
    pwm = constrain(pwm, appcfg::kFanPwmMin, appcfg::kFanPwmMax);
    currentPwm_ = pwm;
    pwmOut_.setPwmByte(currentPwm_);
}

uint8_t FanController::getPwm() const {
    return currentPwm_;
}
