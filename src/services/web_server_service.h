/**
 * @file web_server_service.h
 * @brief Web Server Service for Smart FI Diagnostic Tool.
 * Provides REST API and WebSocket for Live Dashboard.
 */

#ifndef WEB_SERVER_SERVICE_H
#define WEB_SERVER_SERVICE_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SD.h>
#include "config_types.h"

class WebServerService {
public:
    WebServerService(uint16_t port = 80) : server(port), ws("/ws") {}

    void init(const HardwareConfig& hwCfg) {
        // Serve index.html from SD or SPIFFS
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(SD, "/index.html", "text/html");
        });

        // API: Get Live Data
        server.on("/api/live", HTTP_GET, [this](AsyncWebServerRequest *request){
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            DynamicJsonDocument doc(512);
            doc["batt"] = 12.65; // Replace with live data
            doc["rpm"] = 1550;
            doc["tps_v"] = 0.48;
            doc["tps_pct"] = 0;
            doc["temp"] = 85.5;
            serializeJson(doc, *response);
            request->send(response);
        });

        // API: Download Logs
        server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(SD, "/diag_log.csv", "text/csv", true);
        });

        // API: Change Profile
        server.on("/api/profile", HTTP_POST, [](AsyncWebServerRequest *request){
            if (request->hasParam("set")) {
                String profileName = request->getParam("set")->value();
                Serial.println("Changing profile to: " + profileName);
                request->send(200, "text/plain", "Profile changed");
            } else {
                request->send(400, "text/plain", "Missing param");
            }
        });

        // WebSocket Handler
        ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
            if (type == WS_EVT_CONNECT) {
                client->printf("Connected to Smart FI WS");
            }
        });

        server.addHandler(&ws);
        server.begin();
        Serial.println("Web Server Started on port 80");
    }

    void broadcastLiveData(float batt, float rpm, float tps_pct, float temp) {
        DynamicJsonDocument doc(512);
        doc["batt"] = batt;
        doc["rpm"] = rpm;
        doc["tps_pct"] = tps_pct;
        doc["temp"] = temp;
        
        String output;
        serializeJson(doc, output);
        ws.textAll(output);
    }

private:
    AsyncWebServer server;
    AsyncWebSocket ws;
};

#endif // WEB_SERVER_SERVICE_H
