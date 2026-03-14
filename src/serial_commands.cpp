#include "serial_commands.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void SerialCommands::begin() {
    length_ = 0;
    memset(buffer_, 0, sizeof(buffer_));
}

void SerialCommands::process(FanController& fan,
                             const Tachometer::Sample& tachSample,
                             const TempSensors::Sample& tempSample,
                             bool& autoMode) {
    while (Serial.available() > 0) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            buffer_[length_] = '\0';
            handleLine(fan, tachSample, tempSample, autoMode);
            length_ = 0;
            buffer_[0] = '\0';
            continue;
        }

        if (length_ < kBufferSize - 1) {
            buffer_[length_++] = c;
        }
    }
}

void SerialCommands::handleLine(FanController& fan,
                                const Tachometer::Sample& tachSample,
                                const TempSensors::Sample& tempSample,
                                bool& autoMode) {
    trim(buffer_);
    if (buffer_[0] == '\0') {
        return;
    }

    if (strcasecmp(buffer_, "STATUS") == 0) {
        printStatus(fan, tachSample, tempSample, autoMode);
        return;
    }

    if (strcasecmp(buffer_, "HELP") == 0) {
        printHelp();
        return;
    }

    if (strncasecmp(buffer_, "AUTO", 4) == 0) {
        const char* mode = buffer_ + 4;
        while (*mode == ' ' || *mode == ':' || *mode == '=') mode++;
        if (strcasecmp(mode, "ON") == 0 || strcasecmp(mode, "1") == 0) {
            autoMode = true;
            Serial.println("OK AUTO=ON");
            return;
        }
        if (strcasecmp(mode, "OFF") == 0 || strcasecmp(mode, "0") == 0) {
            autoMode = false;
            Serial.println("OK AUTO=OFF");
            return;
        }
        Serial.println("ERR use AUTO ON|OFF");
        return;
    }

    const char* valueText = buffer_;
    if (strncasecmp(buffer_, "PWM", 3) == 0) {
        valueText = buffer_ + 3;
        while (*valueText == ' ' || *valueText == ':' || *valueText == '=') {
            valueText++;
        }
    }

    bool ok = false;
    const int pwmValue = parsePwmValue(valueText, ok);
    if (ok && pwmValue >= 0 && pwmValue <= 255) {
        fan.setPwm(static_cast<uint8_t>(pwmValue));
        autoMode = false;  // manual override
        Serial.print("OK PWM=");
        Serial.println(fan.getPwm());
        return;
    }

    Serial.println("ERR unknown command. Use: STATUS, HELP, PWM:120");
}

int SerialCommands::parsePwmValue(const char* text, bool& ok) {
    ok = false;
    if (text == nullptr || *text == '\0') {
        return 0;
    }

    char* end = nullptr;
    const long value = strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        return 0;
    }
    if (value < 0 || value > 255) {
        return 0;
    }

    ok = true;
    return static_cast<int>(value);
}

void SerialCommands::trim(char* text) {
    if (text == nullptr || *text == '\0') {
        return;
    }

    size_t start = 0;
    size_t end = strlen(text);
    while (start < end && isspace(static_cast<unsigned char>(text[start]))) {
        start++;
    }
    while (end > start && isspace(static_cast<unsigned char>(text[end - 1]))) {
        end--;
    }

    if (start > 0) {
        memmove(text, text + start, end - start);
    }
    text[end - start] = '\0';
}

void SerialCommands::printStatus(const FanController& fan,
                                 const Tachometer::Sample& tachSample,
                                 const TempSensors::Sample& tempSample,
                                 bool autoMode) {
    Serial.print("STATUS pwm=");
    Serial.print(fan.getPwm());
    Serial.print(" auto=");
    Serial.print(autoMode ? "ON" : "OFF");
    Serial.print(" pulses=");
    Serial.print(tachSample.pulses);
    Serial.print(" elapsedMs=");
    Serial.print(tachSample.elapsedMs);
    Serial.print(" rpm=");
    Serial.print(tachSample.rpm);
    Serial.print(" cpuMax=");
    Serial.print(tempSample.hasCpu ? tempSample.maxCpuC : 0.0f, 1);
    Serial.print(" nvmeMax=");
    Serial.print(tempSample.hasNvme ? tempSample.maxNvmeC : 0.0f, 1);
    Serial.print(" delta=");
    Serial.println(tempSample.deltaC, 1);
}

void SerialCommands::printHelp() {
    Serial.println("Commands:");
    Serial.println("  STATUS");
    Serial.println("  HELP");
    Serial.println("  AUTO ON|OFF");
    Serial.println("  PWM:<0..255>");
}
