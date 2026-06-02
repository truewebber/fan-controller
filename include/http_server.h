#pragma once

#include <Arduino.h>
#include <ESP8266WebServer.h>

#include "metrics.h"
#include "runtime_config.h"

class HttpServer {
public:
    void begin(RuntimeConfig* config,
               MetricsSnapshot* metrics,
               bool* restartPending,
               unsigned long* restartAfterMs);

    // Call every loop iteration (non-blocking).
    void update();

private:
    ESP8266WebServer server_{80};
    RuntimeConfig*    config_         = nullptr;
    MetricsSnapshot*  metrics_        = nullptr;
    bool*             restartPending_ = nullptr;
    unsigned long*    restartAfterMs_ = nullptr;

    void handleGetConfig();
    void handlePutConfig();
    void handleGetMetrics();
};
