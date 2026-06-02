#include "metrics.h"

static void appendGaugeInt(String& out, const char* name, long value) {
    out += name;
    out += ' ';
    out += String(value);
    out += '\n';
}

String buildMetricsText(const MetricsSnapshot& snap, const RuntimeConfig& cfg) {
    String out;
    if (!out.reserve(1024)) {
        return F("# error: out of memory\n");
    }

    // Device info
    out += "# HELP fan_controller_info Firmware identification\n";
    out += "# TYPE fan_controller_info gauge\n";
    out += "fan_controller_info{config_version=\"";
    out += cfg.version;
    out += "\"} 1\n";

    // Fan control
    out += "# HELP fan_controller_pwm_duty Current PWM duty cycle (0-255)\n";
    out += "# TYPE fan_controller_pwm_duty gauge\n";
    appendGaugeInt(out, "fan_controller_pwm_duty", snap.pwmDuty);

    out += "# HELP fan_controller_auto_mode 1 = auto, 0 = manual\n";
    out += "# TYPE fan_controller_auto_mode gauge\n";
    appendGaugeInt(out, "fan_controller_auto_mode", snap.autoMode ? 1 : 0);

    // Tachometer
    out += "# HELP fan_controller_tach_rpm Fan speed in RPM\n";
    out += "# TYPE fan_controller_tach_rpm gauge\n";
    appendGaugeInt(out, "fan_controller_tach_rpm", snap.tachRpm);

    out += "# HELP fan_controller_tach_pulses_total Total tach pulses in last interval\n";
    out += "# TYPE fan_controller_tach_pulses_total gauge\n";
    appendGaugeInt(out, "fan_controller_tach_pulses_total", snap.tachPulses);

    // Temperatures
    out += "# HELP fan_controller_temperature_celsius DS18B20 temperature per sensor slot\n";
    out += "# TYPE fan_controller_temperature_celsius gauge\n";
    for (uint8_t i = 0; i < kNumSensors; ++i) {
        if (!snap.tempValid[i]) continue;
        char buf[12];
        out += "fan_controller_temperature_celsius{slot=\"";
        out += cfg.sensors[i].name;
        out += "\"} ";
        out += dtostrf(snap.temperatures[i], 6, 2, buf);
        out += '\n';
    }

    // Wi-Fi
    out += "# HELP fan_controller_wifi_rssi_dbm Wi-Fi signal strength\n";
    out += "# TYPE fan_controller_wifi_rssi_dbm gauge\n";
    appendGaugeInt(out, "fan_controller_wifi_rssi_dbm", snap.wifiRssi);

    return out;
}
