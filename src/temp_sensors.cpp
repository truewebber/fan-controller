#include "temp_sensors.h"

#include "config.h"

#include <string.h>

namespace {
bool validTemp(float t) {
    return t != DEVICE_DISCONNECTED_C && t > -55.0f && t < 125.0f;
}
}  // namespace

TempSensors::TempSensors() : oneWire_(appcfg::kOneWirePin), sensors_(&oneWire_) {}

void TempSensors::begin() {
    sensors_.begin();
    sensors_.setResolution(9);  // Faster conversion, enough for fan control.

    const int count = sensors_.getDeviceCount();
    foundCount_ = (count > appcfg::kTotalSensors) ? appcfg::kTotalSensors : count;

    for (int i = 0; i < appcfg::kTotalSensors; ++i) {
        readings_[i] = 0.0f;
        valid_[i] = false;
        memset(addresses_[i], 0, sizeof(DeviceAddress));
    }

    for (int i = 0; i < foundCount_; ++i) {
        sensors_.getAddress(addresses_[i], i);
    }

    Serial.print("DS18B20 found: ");
    Serial.println(foundCount_);
    for (int i = 0; i < foundCount_; ++i) {
        Serial.print("  [");
        Serial.print(i);
        Serial.print("] ");
        for (int b = 0; b < 8; ++b) {
            if (addresses_[i][b] < 0x10) Serial.print("0");
            Serial.print(addresses_[i][b], HEX);
            if (b < 7) Serial.print(":");
        }
        Serial.println();
    }

    lastReadMs_ = millis();
}

bool TempSensors::update(unsigned long nowMs) {
    if (nowMs - lastReadMs_ < appcfg::kTempReadIntervalMs) {
        return false;
    }

    sensors_.requestTemperatures();
    for (int i = 0; i < appcfg::kTotalSensors; ++i) {
        if (i < foundCount_) {
            const float t = sensors_.getTempC(addresses_[i]);
            valid_[i] = validTemp(t);
            if (valid_[i]) {
                readings_[i] = t;
            }
        } else {
            valid_[i] = false;
        }
    }

    recomputeSample();
    lastReadMs_ = nowMs;
    return true;
}

const TempSensors::Sample& TempSensors::getLastSample() const {
    return lastSample_;
}

int TempSensors::getFoundCount() const {
    return foundCount_;
}

void TempSensors::recomputeSample() {
    Sample s{};

    auto updateMax = [](float v, bool ok, float& maxv, bool& has) {
        if (!ok) return;
        if (!has || v > maxv) {
            maxv = v;
        }
        has = true;
    };

    updateMax(readings_[appcfg::kSensorRpi1Cpu], valid_[appcfg::kSensorRpi1Cpu], s.maxCpuC, s.hasCpu);
    updateMax(readings_[appcfg::kSensorRpi2Cpu], valid_[appcfg::kSensorRpi2Cpu], s.maxCpuC, s.hasCpu);
    updateMax(readings_[appcfg::kSensorRpi3Cpu], valid_[appcfg::kSensorRpi3Cpu], s.maxCpuC, s.hasCpu);
    updateMax(readings_[appcfg::kSensorRpi4Cpu], valid_[appcfg::kSensorRpi4Cpu], s.maxCpuC, s.hasCpu);

    updateMax(readings_[appcfg::kSensorRpi1Nvme], valid_[appcfg::kSensorRpi1Nvme], s.maxNvmeC, s.hasNvme);
    updateMax(readings_[appcfg::kSensorRpi2Nvme], valid_[appcfg::kSensorRpi2Nvme], s.maxNvmeC, s.hasNvme);
    updateMax(readings_[appcfg::kSensorRpi3Nvme], valid_[appcfg::kSensorRpi3Nvme], s.maxNvmeC, s.hasNvme);
    updateMax(readings_[appcfg::kSensorRpi4Nvme], valid_[appcfg::kSensorRpi4Nvme], s.maxNvmeC, s.hasNvme);

    if (valid_[appcfg::kSensorIntake]) {
        s.intakeC = readings_[appcfg::kSensorIntake];
        s.hasIntake = true;
    }
    if (valid_[appcfg::kSensorExhaust]) {
        s.exhaustC = readings_[appcfg::kSensorExhaust];
        s.hasExhaust = true;
    }

    if (s.hasIntake && s.hasExhaust) {
        const float d = s.exhaustC - s.intakeC;
        s.deltaC = d > 0.0f ? d : 0.0f;
    } else {
        s.deltaC = 0.0f;
    }

    lastSample_ = s;
}
