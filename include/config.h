#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

namespace appcfg {

// Serial
constexpr long kBaudRate = 9600;

// Pins (Arduino Pro Mini, ATmega328P 5V/16MHz)
constexpr uint8_t kFanPwmPin = 9;  // OC1A
constexpr uint8_t kTachPin   = 3;  // INT1
constexpr uint8_t kOneWirePin = 2; // GX18B20S bus

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

// Sensors (10x GX18B20S)
constexpr uint8_t kTotalSensors = 10;
constexpr unsigned long kTempReadIntervalMs = 3000;

// Logical sensor slot indices (used to index kSensorAddresses / readings arrays)
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

// 1-Wire ROM addresses for each slot: [family=0x28][6-byte ID][CRC]
// Sensors identified and labeled in temp_sensors.txt
static const uint8_t kSensorAddresses[kTotalSensors][8] = {
    {0x28, 0xC9, 0x1D, 0x7B, 0x02, 0x25, 0x0E, 0xD5},  // 0: kSensorRpi1Cpu
    {0x28, 0x7A, 0xC0, 0x7C, 0x02, 0x25, 0x0E, 0xBF},  // 1: kSensorRpi1Nvme
    {0x28, 0xCD, 0xE1, 0x6A, 0x02, 0x25, 0x0E, 0x39},  // 2: kSensorRpi2Cpu
    {0x28, 0x2E, 0x56, 0x68, 0x02, 0x25, 0x0E, 0x50},  // 3: kSensorRpi2Nvme
    {0x28, 0x2B, 0xFA, 0x5B, 0x02, 0x25, 0x0E, 0x69},  // 4: kSensorRpi3Cpu
    {0x28, 0xC1, 0xE3, 0x4B, 0x02, 0x25, 0x0E, 0x38},  // 5: kSensorRpi3Nvme
    {0x28, 0x1D, 0xD5, 0x76, 0x02, 0x25, 0x0E, 0x4C},  // 6: kSensorRpi4Cpu
    {0x28, 0x45, 0x5B, 0x50, 0x02, 0x25, 0x0E, 0xD3},  // 7: kSensorRpi4Nvme
    {0x28, 0x1D, 0xF8, 0x68, 0x02, 0x25, 0x0E, 0x75},  // 8: kSensorIntake  (front)
    {0x28, 0x5D, 0x20, 0x7A, 0x02, 0x25, 0x0E, 0x5F},  // 9: kSensorExhaust (rear)
};

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

// Debug: uncomment to enable verbose temperature readings each cycle
// #define DEBUG_TEMP_READINGS

}  // namespace appcfg

#endif  // CONFIG_H
