#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>

static constexpr uint8_t kOneWirePin = 14;

OneWire    bus(kOneWirePin);
DallasTemperature sensors(&bus);

static void printAddr(const uint8_t* addr) {
    for (int i = 0; i < 8; ++i) {
        if (addr[i] < 0x10) Serial.print('0');
        Serial.print(addr[i], HEX);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("DS18B20 scanner — touch a sensor to identify it by temperature");
    sensors.begin();
    sensors.setResolution(9);
    sensors.setWaitForConversion(true);
}

void loop() {
    sensors.requestTemperatures();

    int count = sensors.getDeviceCount();
    Serial.print("--- ");
    Serial.print(count);
    Serial.println(" sensor(s) ---");

    for (int i = 0; i < count; ++i) {
        DeviceAddress addr;
        if (!sensors.getAddress(addr, i)) continue;

        float t = sensors.getTempC(addr);

        Serial.print("  [");
        Serial.print(i);
        Serial.print("] ");
        printAddr(addr);
        Serial.print("  ");
        char buf[8];
        Serial.print(dtostrf(t, 5, 1, buf));
        Serial.println(" C");
    }

    delay(2000);
}
