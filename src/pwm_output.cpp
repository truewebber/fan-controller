#include "pwm_output.h"

#include "config.h"

void PwmOutput::begin() {
    pinMode(appcfg::kFanPwmPin, OUTPUT);

    // Timer1 Mode 10: phase-correct PWM, TOP = ICR1, output on OC1A (D9).
    TCCR1A = _BV(COM1A1) | _BV(WGM11);
    TCCR1B = _BV(WGM13) | _BV(CS10);
    ICR1 = appcfg::kPwmTop;
    OCR1A = 0;

    pwmByte_ = 0;
}

void PwmOutput::setPwmByte(uint8_t pwm) {
    pwmByte_ = pwm;
    OCR1A = mapToCompare(pwm);
}

uint8_t PwmOutput::getPwmByte() const {
    return pwmByte_;
}

uint16_t PwmOutput::mapToCompare(uint8_t pwm) const {
    const uint16_t duty = static_cast<uint16_t>(
        (static_cast<uint32_t>(pwm) * appcfg::kPwmTop) / 255U);

    if (appcfg::kPwmActiveLow) {
        return appcfg::kPwmTop - duty;
    }
    return duty;
}
