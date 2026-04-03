#include "temp_sensors.h"

#include "config.h"

#include <string.h>

namespace {
bool validTemp(float t) {
    return t != DEVICE_DISCONNECTED_C && t > -55.0f && t < 125.0f;
}

void printAddress(const uint8_t* addr) {
    for (int b = 0; b < 8; ++b) {
        if (addr[b] < 0x10) Serial.print("0");
        Serial.print(addr[b], HEX);
        if (b < 7) Serial.print(":");
    }
}
}  // namespace

TempSensors::TempSensors() : oneWire_(appcfg::kOneWirePin), sensors_(&oneWire_) {}

void TempSensors::begin() {
    sensors_.begin();
    sensors_.setResolution(9);  // Faster conversion, enough for fan control.

    foundCount_ = 0;
    bool slotMatched[appcfg::kTotalSensors] = {};
    for (int i = 0; i < appcfg::kTotalSensors; ++i) {
        readings_[i] = 0.0f;
        valid_[i] = false;
        memcpy(addresses_[i], appcfg::kSensorAddresses[i], sizeof(DeviceAddress));
    }

    // Enumerate every device on the bus and match against known UUIDs.
    const int busCount = sensors_.getDeviceCount();
    int unknownCount = 0;

    Serial.print("GX18B20S on bus: ");
    Serial.println(busCount);

    for (int bi = 0; bi < busCount; ++bi) {
        DeviceAddress addr;
        if (!sensors_.getAddress(addr, bi)) continue;

        int slot = -1;
        for (int si = 0; si < appcfg::kTotalSensors; ++si) {
            if (memcmp(addr, appcfg::kSensorAddresses[si], sizeof(DeviceAddress)) == 0) {
                slot = si;
                break;
            }
        }

        if (slot >= 0) {
            slotMatched[slot] = true;
            ++foundCount_;
            Serial.print("  [");
            Serial.print(slot);
            Serial.print("] ");
            printAddress(addr);
            Serial.println(" OK");
        } else {
            Serial.print("  [?] ");
            printAddress(addr);
            Serial.println(" UNKNOWN");
            ++unknownCount;
        }
    }

    // Report configured sensors that were not seen on the bus.
    for (int si = 0; si < appcfg::kTotalSensors; ++si) {
        if (!slotMatched[si]) {
            Serial.print("  [");
            Serial.print(si);
            Serial.print("] ");
            printAddress(appcfg::kSensorAddresses[si]);
            Serial.println(" NOT FOUND");
        }
    }

    Serial.print("Known: ");
    Serial.print(foundCount_);
    Serial.print("/");
    Serial.print(appcfg::kTotalSensors);
    Serial.print(", Unknown: ");
    Serial.println(unknownCount);

    lastReadMs_ = millis();
}

bool TempSensors::update(unsigned long nowMs) {
    if (nowMs - lastReadMs_ < appcfg::kTempReadIntervalMs) {
        return false;
    }

    sensors_.requestTemperatures();
    for (int i = 0; i < appcfg::kTotalSensors; ++i) {
        const float t = sensors_.getTempC(addresses_[i]);
        valid_[i] = validTemp(t);
        if (valid_[i]) {
            readings_[i] = t;
        }

#ifdef DEBUG_TEMP_READINGS
        Serial.print("  DBG [");
        Serial.print(i);
        Serial.print("] ");
        printAddress(addresses_[i]);
        Serial.print(" -> ");
        if (valid_[i]) {
            Serial.print(t, 2);
            Serial.println(" C");
        } else {
            Serial.println("INVALID");
        }
#endif
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
