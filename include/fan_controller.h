#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include <Arduino.h>

#include "pwm_output.h"

class FanController {
public:
    void begin();
    void setPwm(uint8_t pwm);
    uint8_t getPwm() const;

private:
    PwmOutput pwmOut_{};
    uint8_t currentPwm_ = 0;
};

#endif  // FAN_CONTROLLER_H
