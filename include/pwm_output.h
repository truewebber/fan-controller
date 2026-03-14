#ifndef PWM_OUTPUT_H
#define PWM_OUTPUT_H

#include <Arduino.h>

class PwmOutput {
public:
    void begin();
    void setPwmByte(uint8_t pwm);
    uint8_t getPwmByte() const;

private:
    uint8_t pwmByte_ = 0;
    uint16_t mapToCompare(uint8_t pwm) const;
};

#endif  // PWM_OUTPUT_H
