#include "http_server.h"


static constexpr unsigned long kRestartDelayMs = 1000;


void HttpServer::begin(RuntimeConfig* config,
                       MetricsSnapshot* metrics,
                       bool* restartPending,
                       unsigned long* restartAfterMs) {
    config_         = config;
    metrics_        = metrics;
    restartPending_ = restartPending;
    restartAfterMs_ = restartAfterMs;

    server_.on("/config",  HTTP_GET,  [this]() { handleGetConfig();  });
    server_.on("/config",  HTTP_PUT,  [this]() { handlePutConfig();  });
    server_.on("/metrics", HTTP_GET,  [this]() { handleGetMetrics(); });
    server_.onNotFound([this]() {
        server_.send(404, "text/plain", "Not found\n");
    });

    server_.begin();
    Serial.println("[http] server started on port 80");
    Serial.println("[http]  GET  /config");
    Serial.println("[http]  PUT  /config");
    Serial.println("[http]  GET  /metrics");
}

void HttpServer::update() {
    server_.handleClient();
}

void HttpServer::handleGetConfig() {
    server_.send(200, "application/json", serializeConfig(*config_));
}

void HttpServer::handlePutConfig() {
    const String body = server_.arg("plain");
    if (body.isEmpty()) {
        server_.send(400, "application/json", R"({"error":"Empty body"})");
        return;
    }

    RuntimeConfig newCfg = defaultConfig();
    String error;
    if (!parseConfig(body, newCfg, error)) {
        const String resp = String(R"({"error":")") + error + "\"}";
        server_.send(400, "application/json", resp);
        return;
    }

    if (!saveConfig(newCfg)) {
        server_.send(500, "application/json", R"({"error":"Failed to save config"})");
        return;
    }

    *config_ = newCfg;
    server_.send(200, "application/json", R"({"ok":true,"restart":true})");

    *restartPending_  = true;
    *restartAfterMs_  = millis() + kRestartDelayMs;
}

void HttpServer::handleGetMetrics() {
    server_.send(200, "text/plain; version=0.0.4", buildMetricsText(*metrics_, *config_));
}
