#pragma once

#include <Arduino.h>

#include "runtime_config.h"

struct MetricsSnapshot {
    unsigned long capturedAtMs = 0;
    uint8_t pwmDuty            = 0;
    bool autoMode              = false;
    unsigned int tachRpm       = 0;
    unsigned long tachPulses   = 0;
    float temperatures[kNumSensors];
    bool tempValid[kNumSensors];
    int32_t wifiRssi = 0;

    MetricsSnapshot() {
        for (uint8_t i = 0; i < kNumSensors; ++i) {
            temperatures[i] = 0.0f;
            tempValid[i]    = false;
        }
    }
};

// Build Prometheus-formatted text from snapshot + config (for sensor label names).
String buildMetricsText(const MetricsSnapshot& snap, const RuntimeConfig& cfg);
