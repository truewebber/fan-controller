#include <Arduino.h>
#include <LittleFS.h>
#include <math.h>

#include "fan_controller_esp.h"
#include "http_server.h"
#include "metrics.h"
#include "runtime_config.h"
#include "tachometer_esp.h"
#include "temp_sensors_esp.h"
#include "wifi_manager.h"

// ---------------------------------------------------------------------------
// Application state
// ---------------------------------------------------------------------------

namespace {

RuntimeConfig    cfg;
FanControllerEsp fanController;
TachometerEsp    tachometer;
TempSensorsEsp   tempSensors;
WifiManager      wifi;
HttpServer       httpServer;
MetricsSnapshot  metricsSnap;

bool autoMode        = true;
bool restartPending  = false;
unsigned long restartAfterMs = 0;

unsigned long lastStatusMs  = 0;
unsigned long lastMetricsMs = 0;

// ---------------------------------------------------------------------------
// Fan curve
// ---------------------------------------------------------------------------

float mapTempToDemand(float value, float minV, float maxV) {
    if (value <= minV) return 0.0f;
    if (value >= maxV) return 1.0f;
    const float ratio = (value - minV) / (maxV - minV);
    return powf(ratio, cfg.fanCurveExp);
}

uint8_t computeAutoPwm(const TempSensorsEsp::Sample& s) {
    const float cpu   = s.hasCpu  ? mapTempToDemand(s.maxCpuC,  cfg.cpuTempMinC,  cfg.cpuTempMaxC)  : 0.0f;
    const float nvme  = s.hasNvme ? mapTempToDemand(s.maxNvmeC, cfg.nvmeTempMinC, cfg.nvmeTempMaxC) : 0.0f;
    const float delta = (s.hasIntake && s.hasExhaust)
                            ? mapTempToDemand(s.deltaC, cfg.deltaTempMinC, cfg.deltaTempMaxC)
                            : 0.0f;
    const float demand = max(cpu, max(nvme, delta));
    const float pwm    = cfg.fanPwmMin + demand * (cfg.fanPwmMax - cfg.fanPwmMin);
    return static_cast<uint8_t>(constrain(static_cast<int>(pwm + 0.5f),
                                          cfg.fanPwmMin, cfg.fanPwmMax));
}

// ---------------------------------------------------------------------------
// Serial CLI
// ---------------------------------------------------------------------------

void handleSerial(const char* line) {
    if (strcasecmp(line, "STATUS") == 0) {
        const auto& ts  = tachometer.getLastSample();
        const auto& tmp = tempSensors.getLastSample();
        Serial.print("STATUS pwm=");     Serial.print(fanController.getPwm());
        Serial.print(" auto=");          Serial.print(autoMode ? "ON" : "OFF");
        Serial.print(" rpm=");           Serial.print(ts.rpm);
        char fb[12];
        Serial.print(" cpu=");   Serial.print(dtostrf(tmp.hasCpu  ? tmp.maxCpuC  : 0.0f, 4, 1, fb));
        Serial.print(" nvme=");  Serial.print(dtostrf(tmp.hasNvme ? tmp.maxNvmeC : 0.0f, 4, 1, fb));
        Serial.print(" delta="); Serial.println(dtostrf(tmp.deltaC, 4, 1, fb));
        return;
    }
    if (strcasecmp(line, "HELP") == 0) {
        Serial.println("Commands: STATUS, HELP, AUTO ON|OFF, PWM:<0-255>, IP");
        return;
    }
    if (strcasecmp(line, "IP") == 0) {
        Serial.println(wifi.localIP());
        return;
    }
    if (strncasecmp(line, "AUTO", 4) == 0) {
        const char* arg = line + 4;
        while (*arg == ' ' || *arg == ':') ++arg;
        if (strcasecmp(arg, "ON") == 0)  { autoMode = true;  Serial.println("OK AUTO=ON");  return; }
        if (strcasecmp(arg, "OFF") == 0) { autoMode = false; Serial.println("OK AUTO=OFF"); return; }
        Serial.println("ERR: AUTO ON|OFF");
        return;
    }
    if (strncasecmp(line, "PWM", 3) == 0) {
        const char* arg = line + 3;
        while (*arg == ' ' || *arg == ':' || *arg == '=') ++arg;
        char* end;
        const long v = strtol(arg, &end, 10);
        if (end != arg && *end == '\0' && v >= 0 && v <= 255) {
            fanController.setPwm(static_cast<uint8_t>(v));
            autoMode = false;
            Serial.print("OK PWM="); Serial.println(fanController.getPwm());
        } else {
            Serial.println("ERR: PWM:<0-255>");
        }
        return;
    }
    Serial.println("ERR: unknown command");
}

void processSerial() {
    static char buf[64];
    static uint8_t len = 0;
    while (Serial.available()) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\r') continue;
        if (c == '\n') {
            buf[len] = '\0';
            if (len > 0) handleSerial(buf);
            len = 0;
            continue;
        }
        if (len < sizeof(buf) - 1) buf[len++] = c;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// setup / loop
// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("fan-controller ESP-12F starting...");

    if (!LittleFS.begin()) {
        Serial.println("[fs] mount failed — formatting...");
        LittleFS.format();
        LittleFS.begin();
    }

    cfg = defaultConfig();
    loadConfig(cfg);

    fanController.begin(cfg);
    tachometer.begin(cfg);
    tempSensors.begin(cfg);

    wifi.connect(cfg.wifiSsid, cfg.wifiPassword);

    httpServer.begin(&cfg, &metricsSnap, &restartPending, &restartAfterMs);

    Serial.println("System ready.");
}

void loop() {
    const unsigned long now = millis();

    wifi.maintain();
    httpServer.update();
    tachometer.update(now);

    const bool newTemp = tempSensors.update(now);
    if (autoMode && newTemp) {
        fanController.setPwm(computeAutoPwm(tempSensors.getLastSample()));
    }

    processSerial();

    if (now - lastMetricsMs >= 1000) {
        const auto& ts  = tachometer.getLastSample();
        metricsSnap.capturedAtMs = now;
        metricsSnap.pwmDuty      = fanController.getPwm();
        metricsSnap.autoMode     = autoMode;
        metricsSnap.tachRpm      = ts.rpm;
        metricsSnap.tachPulses   = ts.pulses;
        metricsSnap.wifiRssi     = wifi.rssi();
        for (uint8_t i = 0; i < kNumSensors; ++i) {
            metricsSnap.temperatures[i] = tempSensors.getReading(i);
            metricsSnap.tempValid[i]    = tempSensors.isValid(i);
        }
        lastMetricsMs = now;
    }

    if (now - lastStatusMs >= cfg.statusPrintIntervalMs) {
        const auto& ts  = tachometer.getLastSample();
        const auto& tmp = tempSensors.getLastSample();
        Serial.print("STATUS pwm=");  Serial.print(fanController.getPwm());
        Serial.print(" auto=");       Serial.print(autoMode ? "ON" : "OFF");
        Serial.print(" rpm=");        Serial.print(ts.rpm);
        char fb[12];
        Serial.print(" cpu=");   Serial.print(dtostrf(tmp.hasCpu  ? tmp.maxCpuC  : 0.0f, 4, 1, fb));
        Serial.print(" nvme=");  Serial.print(dtostrf(tmp.hasNvme ? tmp.maxNvmeC : 0.0f, 4, 1, fb));
        Serial.print(" delta="); Serial.print(dtostrf(tmp.deltaC, 4, 1, fb));
        Serial.print(" rssi=");       Serial.println(wifi.rssi());
        lastStatusMs = now;
    }

    if (restartPending && now >= restartAfterMs) {
        Serial.println("[sys] restarting...");
        delay(100);
        ESP.restart();
    }
}
