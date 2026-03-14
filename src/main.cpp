#include <Arduino.h>
#include <math.h>

#include "config.h"
#include "fan_controller.h"
#include "serial_commands.h"
#include "tachometer.h"
#include "temp_sensors.h"

namespace {
FanController fanController;
Tachometer tachometer;
TempSensors tempSensors;
SerialCommands serialCommands;
unsigned long lastStatusPrintMs = 0;
bool autoMode = true;

float mapTempToDemand(float value, float minV, float maxV) {
    if (value <= minV) return 0.0f;
    if (value >= maxV) return 1.0f;
    const float ratio = (value - minV) / (maxV - minV);
    return powf(ratio, appcfg::kFanCurveExponent);
}

uint8_t computeAutoPwm(const TempSensors::Sample& s) {
    const float cpuDemand = s.hasCpu ? mapTempToDemand(s.maxCpuC, appcfg::kCpuTempMinC, appcfg::kCpuTempMaxC) : 0.0f;
    const float nvmeDemand = s.hasNvme ? mapTempToDemand(s.maxNvmeC, appcfg::kNvmeTempMinC, appcfg::kNvmeTempMaxC) : 0.0f;
    const float deltaDemand = (s.hasIntake && s.hasExhaust) ? mapTempToDemand(s.deltaC, appcfg::kDeltaTempMinC, appcfg::kDeltaTempMaxC) : 0.0f;
    const float demand = max(cpuDemand, max(nvmeDemand, deltaDemand));
    const float pwm = appcfg::kFanPwmMin + demand * (appcfg::kFanPwmMax - appcfg::kFanPwmMin);
    return static_cast<uint8_t>(constrain(static_cast<int>(pwm + 0.5f), appcfg::kFanPwmMin, appcfg::kFanPwmMax));
}
}

void setup() {
    Serial.begin(appcfg::kBaudRate);
    Serial.println("Fan + Tach controller starting...");

    fanController.begin();
    tachometer.begin();
    tempSensors.begin();
    serialCommands.begin();

    Serial.println("System ready.");
    Serial.println("Commands: STATUS, HELP, AUTO ON|OFF, PWM:<0-255> | PWM mode: 25kHz Timer1");
}

void loop() {
    const unsigned long now = millis();
    tachometer.update(now);
    const bool hasTempUpdate = tempSensors.update(now);

    const Tachometer::Sample& sample = tachometer.getLastSample();
    const TempSensors::Sample& temp = tempSensors.getLastSample();
    serialCommands.process(fanController, sample, temp, autoMode);

    if (autoMode && hasTempUpdate) {
        fanController.setPwm(computeAutoPwm(temp));
    }

    if (now - lastStatusPrintMs >= appcfg::kStatusPrintIntervalMs) {
        Serial.print("STATUS pwm=");
        Serial.print(fanController.getPwm());
        Serial.print(" auto=");
        Serial.print(autoMode ? "ON" : "OFF");
        Serial.print(" pulses=");
        Serial.print(sample.pulses);
        Serial.print(" elapsedMs=");
        Serial.print(sample.elapsedMs);
        Serial.print(" rpm=");
        Serial.print(sample.rpm);
        Serial.print(" cpuMax=");
        Serial.print(temp.hasCpu ? temp.maxCpuC : 0.0f, 1);
        Serial.print(" nvmeMax=");
        Serial.print(temp.hasNvme ? temp.maxNvmeC : 0.0f, 1);
        Serial.print(" delta=");
        Serial.println(temp.deltaC, 1);
        lastStatusPrintMs = now;
    }
}
