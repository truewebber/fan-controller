#pragma once

#include <Arduino.h>

class WifiManager {
public:
    // Attempt connection; blocks up to timeoutMs. Returns true if connected.
    bool connect(const char* ssid, const char* password,
                 unsigned long timeoutMs = 20000);

    // Call every loop iteration — reconnects silently if dropped.
    void maintain();

    bool isConnected() const;
    int32_t rssi() const;
    String localIP() const;
};
