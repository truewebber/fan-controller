#ifndef SERIAL_COMMANDS_H
#define SERIAL_COMMANDS_H

#include <Arduino.h>

#include "fan_controller.h"
#include "tachometer.h"
#include "temp_sensors.h"

class SerialCommands {
public:
    void begin();
    void process(FanController& fan,
                 const Tachometer::Sample& tachSample,
                 const TempSensors::Sample& tempSample,
                 bool& autoMode);

private:
    static constexpr uint8_t kBufferSize = 32;
    char buffer_[kBufferSize] = {0};
    uint8_t length_ = 0;

    void handleLine(FanController& fan,
                    const Tachometer::Sample& tachSample,
                    const TempSensors::Sample& tempSample,
                    bool& autoMode);
    static int parsePwmValue(const char* text, bool& ok);
    static void trim(char* text);
    static void printStatus(const FanController& fan,
                            const Tachometer::Sample& tachSample,
                            const TempSensors::Sample& tempSample,
                            bool autoMode);
    static void printHelp();
};

#endif  // SERIAL_COMMANDS_H
