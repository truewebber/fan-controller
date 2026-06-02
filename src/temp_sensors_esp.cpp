#include "temp_sensors_esp.h"

#include <string.h>

namespace {
bool validTemp(float t) {
    return t != DEVICE_DISCONNECTED_C && t > -55.0f && t < 125.0f;
}
}  // namespace

TempSensorsEsp::~TempSensorsEsp() {
    delete sensors_;
    delete oneWire_;
}

void TempSensorsEsp::begin(const RuntimeConfig& cfg) {
    intervalMs_ = cfg.tempReadIntervalMs;

    delete sensors_;
    delete oneWire_;
    oneWire_ = new OneWire(cfg.oneWirePin);
    sensors_ = new DallasTemperature(oneWire_);

    sensors_->begin();
    sensors_->setResolution(9);  // 9-bit: ~94 ms conversion, sufficient for fan ctrl.
    sensors_->setWaitForConversion(true);

    foundCount_ = 0;
    for (uint8_t i = 0; i < kNumSensors; ++i) {
        readings_[i] = 0.0f;
        valid_[i]    = false;
        memcpy(addresses_[i], cfg.sensors[i].address, sizeof(DeviceAddress));
    }

    const int busCount = sensors_->getDeviceCount();
    Serial.print("[temp] DS18B20 on bus: ");
    Serial.println(busCount);

    bool matched[kNumSensors] = {};
    for (int bi = 0; bi < busCount; ++bi) {
        DeviceAddress addr;
        if (!sensors_->getAddress(addr, bi)) continue;

        int hit = -1;
        for (uint8_t si = 0; si < kNumSensors; ++si) {
            if (memcmp(addr, addresses_[si], sizeof(DeviceAddress)) == 0) {
                hit = si;
                break;
            }
        }
        if (hit >= 0) {
            matched[hit] = true;
            ++foundCount_;
            Serial.print("  [");
            Serial.print(hit);
            Serial.print("] ");
            Serial.print(cfg.sensors[hit].name);
            Serial.println(" OK");
        }
    }

    for (uint8_t si = 0; si < kNumSensors; ++si) {
        if (!matched[si]) {
            Serial.print("  [");
            Serial.print(si);
            Serial.print("] ");
            Serial.print(cfg.sensors[si].name);
            Serial.println(" NOT FOUND");
        }
    }

    Serial.print("[temp] found ");
    Serial.print(foundCount_);
    Serial.print("/");
    Serial.println(kNumSensors);

    lastReadMs_ = millis();
}

bool TempSensorsEsp::update(unsigned long nowMs) {
    if (nowMs - lastReadMs_ < intervalMs_) return false;

    // requestTemperatures() with waitForConversion=true blocks ~94 ms.
    // On ESP8266 the internal delay() call yields to the scheduler/WDT.
    sensors_->requestTemperatures();

    for (uint8_t i = 0; i < kNumSensors; ++i) {
        const float t = sensors_->getTempC(addresses_[i]);
        valid_[i]    = validTemp(t);
        if (valid_[i]) readings_[i] = t;
    }

    recomputeSample();
    lastReadMs_ = nowMs;
    return true;
}

const TempSensorsEsp::Sample& TempSensorsEsp::getLastSample() const {
    return lastSample_;
}

float TempSensorsEsp::getReading(uint8_t slot) const {
    if (slot >= kNumSensors || !valid_[slot]) return NAN;
    return readings_[slot];
}

bool TempSensorsEsp::isValid(uint8_t slot) const {
    return slot < kNumSensors && valid_[slot];
}

int TempSensorsEsp::getFoundCount() const {
    return foundCount_;
}

void TempSensorsEsp::recomputeSample() {
    Sample s{};

    auto updateMax = [](float v, bool ok, float& maxv, bool& has) {
        if (!ok) return;
        if (!has || v > maxv) maxv = v;
        has = true;
    };

    updateMax(readings_[slot::kRpi1Cpu],  valid_[slot::kRpi1Cpu],  s.maxCpuC, s.hasCpu);
    updateMax(readings_[slot::kRpi2Cpu],  valid_[slot::kRpi2Cpu],  s.maxCpuC, s.hasCpu);
    updateMax(readings_[slot::kRpi3Cpu],  valid_[slot::kRpi3Cpu],  s.maxCpuC, s.hasCpu);
    updateMax(readings_[slot::kRpi4Cpu],  valid_[slot::kRpi4Cpu],  s.maxCpuC, s.hasCpu);

    updateMax(readings_[slot::kRpi1Nvme], valid_[slot::kRpi1Nvme], s.maxNvmeC, s.hasNvme);
    updateMax(readings_[slot::kRpi2Nvme], valid_[slot::kRpi2Nvme], s.maxNvmeC, s.hasNvme);
    updateMax(readings_[slot::kRpi3Nvme], valid_[slot::kRpi3Nvme], s.maxNvmeC, s.hasNvme);
    updateMax(readings_[slot::kRpi4Nvme], valid_[slot::kRpi4Nvme], s.maxNvmeC, s.hasNvme);

    if (valid_[slot::kIntake]) {
        s.intakeC   = readings_[slot::kIntake];
        s.hasIntake = true;
    }
    if (valid_[slot::kExhaust]) {
        s.exhaustC   = readings_[slot::kExhaust];
        s.hasExhaust = true;
    }
    if (s.hasIntake && s.hasExhaust) {
        const float d = s.exhaustC - s.intakeC;
        s.deltaC = d > 0.0f ? d : 0.0f;
    }

    lastSample_ = s;
}
