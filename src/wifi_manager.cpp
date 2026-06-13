#include "wifi_manager.h"

#include <ESP8266WiFi.h>

bool WifiManager::connect(const char* ssid, const char* password,
                          unsigned long timeoutMs) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("[wifi] connecting to ");
    Serial.print(ssid);

    const unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= timeoutMs) {
            Serial.println();
            Serial.println("[wifi] timeout");
            return false;
        }
        delay(500);
        Serial.print('.');
    }
    Serial.println();
    Serial.print("[wifi] connected, IP: ");
    Serial.println(WiFi.localIP());
    return true;
}

void WifiManager::maintain() {
    if (WiFi.status() == WL_CONNECTED) return;

    const unsigned long now = millis();
    if (now - lastReconnectMs_ < kReconnectIntervalMs) return;

    lastReconnectMs_ = now;
    Serial.println("[wifi] reconnecting...");
    WiFi.reconnect();
}

bool WifiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

int32_t WifiManager::rssi() const {
    return WiFi.RSSI();
}

String WifiManager::localIP() const {
    return WiFi.localIP().toString();
}
