#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

namespace appcfg {

// Serial
constexpr long kBaudRate = 9600;

// Pins (Arduino Pro Mini, ATmega328P 5V/16MHz)
constexpr uint8_t kFanPwmPin = 9;  // OC1A
constexpr uint8_t kTachPin   = 3;  // INT1
constexpr uint8_t kOneWirePin = 2; // DS18B20 bus

// Fan range in user-facing units (0..255)
constexpr uint8_t kFanPwmMin = 30;
constexpr uint8_t kFanPwmMax = 255;

// Timer1 for 25kHz phase-correct PWM:
// f_pwm = f_clk / (2 * N * TOP) = 16e6 / (2 * 1 * 320) = 25kHz
constexpr uint16_t kPwmTop = 320;

// Set true only if your electrical stage inverts PWM (e.g. NPN open-collector path).
constexpr bool kPwmActiveLow = false;

// Tachometer
constexpr uint8_t kTachPulsesPerRevolution = 2;
constexpr unsigned long kRpmCalcIntervalMs = 1000;

// Sensors (10x DS18B20)
constexpr uint8_t kTotalSensors = 10;
constexpr unsigned long kTempReadIntervalMs = 3000;

// Sensor index mapping on OneWire bus
constexpr uint8_t kSensorRpi1Cpu  = 0;
constexpr uint8_t kSensorRpi1Nvme = 1;
constexpr uint8_t kSensorRpi2Cpu  = 2;
constexpr uint8_t kSensorRpi2Nvme = 3;
constexpr uint8_t kSensorRpi3Cpu  = 4;
constexpr uint8_t kSensorRpi3Nvme = 5;
constexpr uint8_t kSensorRpi4Cpu  = 6;
constexpr uint8_t kSensorRpi4Nvme = 7;
constexpr uint8_t kSensorIntake   = 8;
constexpr uint8_t kSensorExhaust  = 9;

// Auto-control thresholds
constexpr float kCpuTempMinC = 40.0f;
constexpr float kCpuTempMaxC = 75.0f;
constexpr float kNvmeTempMinC = 40.0f;
constexpr float kNvmeTempMaxC = 70.0f;
constexpr float kDeltaTempMinC = 5.0f;
constexpr float kDeltaTempMaxC = 15.0f;
constexpr float kFanCurveExponent = 2.5f;

// Status print
constexpr unsigned long kStatusPrintIntervalMs = 5000;

}  // namespace appcfg

#endif  // CONFIG_H
