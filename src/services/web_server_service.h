/**
 * @file web_server_service.h
 * @brief Web Server Service for Smart FI Diagnostic Tool.
 * Provides REST API, WebSocket for Live Dashboard, TPS Calibration, Log Manager, and Acceleration Timer.
 */

#ifndef WEB_SERVER_SERVICE_H
#define WEB_SERVER_SERVICE_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SD.h>
#include "config_types.h"
#include "core/fs_manager.h"
#include "core/memory_manager.h"
#include "logging_service.h"
#include "acceleration_timer.h"

// Mutex for protecting shared String data between async web handlers and main loop
static portMUX_TYPE webDataMux = portMUX_INITIALIZER_UNLOCKED;

class WebServerService {
public:
    WebServerService(uint16_t port = 80) : server(port), ws("/ws"), 
        reportRequested(false), dtcClearRequested(false) {}

    void init(const HardwareConfig& hwCfg) {
        // ==========================================
        // STATIC FILES (SPIFFS / SD dual storage)
        // ==========================================
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(FSManager::getInstance().getWebFS(), "/index.html", "text/html");
        });

        server.on("/dashboard.js", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(FSManager::getInstance().getWebFS(), "/dashboard.js", "application/javascript");
        });

        server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(FSManager::getInstance().getWebFS(), "/style.css", "text/css");
        });

        // ==========================================
        // LIVE DATA API
        // ==========================================
        server.on("/api/live", HTTP_GET, [this](AsyncWebServerRequest *request){
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            StaticJsonDocument<256> doc;
            doc["batt"] = lastBatt;
            doc["rpm"] = lastRPM;
            doc["tps_pct"] = lastTPS;
            doc["temp"] = lastTemp;
            doc["fuel_rate"] = lastFuelRate;
            doc["fuel_total"] = lastFuelTotal;
            serializeJson(doc, *response);
            request->send(response);
        });

        // ==========================================
        // HEALTH SCORE API
        // ==========================================
        server.on("/api/health", HTTP_GET, [this](AsyncWebServerRequest *request){
            portENTER_CRITICAL(&webDataMux);
            String copy = healthJson;
            portEXIT_CRITICAL(&webDataMux);
            request->send(200, "application/json", copy);
        });

        // ==========================================
        // DTC API
        // ==========================================
        server.on("/api/dtc", HTTP_GET, [this](AsyncWebServerRequest *request){
            portENTER_CRITICAL(&webDataMux);
            String copy = dtcJson;
            portEXIT_CRITICAL(&webDataMux);
            request->send(200, "application/json", copy);
        });

        server.on("/api/dtc/clear", HTTP_POST, [this](AsyncWebServerRequest *request){
            dtcClearRequested = true;
            request->send(200, "text/plain", "DTC clear requested");
        });

        // ==========================================
        // FREEZE FRAME API
        // ==========================================
        server.on("/api/freeze", HTTP_GET, [this](AsyncWebServerRequest *request){
            portENTER_CRITICAL(&webDataMux);
            String copy = freezeJson;
            portEXIT_CRITICAL(&webDataMux);
            request->send(200, "application/json", copy);
        });

        server.on("/api/freeze/capture", HTTP_POST, [this](AsyncWebServerRequest *request){
            manualFreezeRequested = true;
            request->send(200, "text/plain", "Freeze frame capture requested");
        });

        // ==========================================
        // TREND DATA API
        // ==========================================
        server.on("/api/trend", HTTP_GET, [this](AsyncWebServerRequest *request){
            portENTER_CRITICAL(&webDataMux);
            String copy = trendJson;
            portEXIT_CRITICAL(&webDataMux);
            request->send(200, "application/json", copy);
        });

        // ==========================================
        // CRANKING ANALYSIS API
        // ==========================================
        server.on("/api/crank", HTTP_GET, [this](AsyncWebServerRequest *request){
            portENTER_CRITICAL(&webDataMux);
            String copy = crankJson;
            portEXIT_CRITICAL(&webDataMux);
            request->send(200, "application/json", copy);
        });

        // ==========================================
        // DIAGNOSTIC WIZARD API
        // ==========================================
        server.on("/api/wizard/symptoms", HTTP_GET, [this](AsyncWebServerRequest *request){
            portENTER_CRITICAL(&webDataMux);
            String copy = wizardSymptomsJson;
            portEXIT_CRITICAL(&webDataMux);
            request->send(200, "application/json", copy);
        });

        server.on("/api/wizard/analyze", HTTP_GET, [this](AsyncWebServerRequest *request){
            if (request->hasParam("id")) {
                wizardSymptomId = request->getParam("id")->value().toInt();
                wizardAnalyzeRequested = true;
                request->send(200, "text/plain", "Analysis started");
            } else {
                request->send(400, "text/plain", "Missing symptom id");
            }
        });

        server.on("/api/wizard/result", HTTP_GET, [this](AsyncWebServerRequest *request){
            portENTER_CRITICAL(&webDataMux);
            String copy = wizardResultJson;
            portEXIT_CRITICAL(&webDataMux);
            request->send(200, "application/json", copy);
        });

        // ==========================================
        // FUEL ESTIMATOR API
        // ==========================================
        server.on("/api/fuel", HTTP_GET, [this](AsyncWebServerRequest *request){
            StaticJsonDocument<128> doc;
            doc["rate_cc_min"] = serialized(String(lastFuelRate, 2));
            doc["total_cc"] = serialized(String(lastFuelTotal, 1));
            char buf[96];
            size_t len = serializeJson(doc, buf, sizeof(buf));
            request->send(200, "application/json", String(buf, len));
        });

        server.on("/api/fuel/reset", HTTP_POST, [this](AsyncWebServerRequest *request){
            fuelResetRequested = true;
            request->send(200, "text/plain", "Fuel trip reset");
        });

        // ==========================================
        // REPORT API
        // ==========================================
        server.on("/api/report/generate", HTTP_POST, [this](AsyncWebServerRequest *request){
            reportRequested = true;
            request->send(200, "text/plain", "Report generation started");
        });

        server.on("/api/report/download", HTTP_GET, [this](AsyncWebServerRequest *request){
            portENTER_CRITICAL(&webDataMux);
            String file = lastReportFile;
            portEXIT_CRITICAL(&webDataMux);
            if (file.isEmpty()) {
                request->send(404, "text/plain", "No report available");
            } else {
                request->send(SD, file, "text/html", true);
            }
        });

        // ==========================================
        // PROFILE API
        // ==========================================
        server.on("/api/profile", HTTP_GET, [this](AsyncWebServerRequest *request){
            portENTER_CRITICAL(&webDataMux);
            String copy = profileJson;
            portEXIT_CRITICAL(&webDataMux);
            request->send(200, "application/json", copy);
        });

        server.on("/api/profile/change", HTTP_POST, [this](AsyncWebServerRequest *request){
            if (request->hasParam("name")) {
                pendingProfile = request->getParam("name")->value();
                profileChangeRequested = true;
                request->send(200, "text/plain", "Profile change requested (reboot needed)");
            } else {
                request->send(400, "text/plain", "Missing profile name");
            }
        });

        // ==========================================
        // LOG MANAGER API (List & Download CSVs)
        // ==========================================
        server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *request){
            if (request->hasParam("file")) {
                String reqFile = request->getParam("file")->value();
                if (!reqFile.startsWith("/")) reqFile = "/" + reqFile;
                if (SD.exists(reqFile)) {
                    request->send(SD, reqFile, "text/csv", true);
                    return;
                }
            }
            if (SD.exists("/diag_log.csv")) {
                request->send(SD, "/diag_log.csv", "text/csv", true);
            } else {
                request->send(404, "text/plain", "Log file not found");
            }
        });

        server.on("/api/logs/list", HTTP_GET, [](AsyncWebServerRequest *request){
            String listJson = LoggingService::getInstance().getLogFilesJson();
            request->send(200, "application/json", listJson);
        });

        // ==========================================
        // TPS CALIBRATION API
        // ==========================================
        server.on("/api/tps/status", HTTP_GET, [this](AsyncWebServerRequest *request){
            portENTER_CRITICAL(&webDataMux);
            String copy = tpsStatusJson;
            portEXIT_CRITICAL(&webDataMux);
            request->send(200, "application/json", copy);
        });

        server.on("/api/tps/calibrate_zero", HTTP_POST, [this](AsyncWebServerRequest *request){
            tpsCalibrateZeroRequested = true;
            request->send(200, "text/plain", "Zero throttle calibration requested");
        });

        server.on("/api/tps/calibrate_wot", HTTP_POST, [this](AsyncWebServerRequest *request){
            tpsCalibrateWotRequested = true;
            request->send(200, "text/plain", "WOT calibration requested");
        });

        server.on("/api/tps/reset", HTTP_POST, [this](AsyncWebServerRequest *request){
            tpsResetRequested = true;
            request->send(200, "text/plain", "TPS calibration reset requested");
        });

        // ==========================================
        // VIRTUAL ACCELERATION 0-60 TIMER API
        // ==========================================
        server.on("/api/accel", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(200, "application/json", AccelerationTimer::getInstance().toJson());
        });

        server.on("/api/accel/arm", HTTP_POST, [](AsyncWebServerRequest *request){
            AccelerationTimer::getInstance().arm();
            request->send(200, "text/plain", "Acceleration timer armed");
        });

        server.on("/api/accel/reset", HTTP_POST, [](AsyncWebServerRequest *request){
            AccelerationTimer::getInstance().reset();
            request->send(200, "text/plain", "Acceleration timer reset");
        });

        // ==========================================
        // SYSTEM INFO / MEMORY API
        // ==========================================
        server.on("/api/system", HTTP_GET, [](AsyncWebServerRequest *request){
            StaticJsonDocument<256> doc;
            doc["free_heap"] = ESP.getFreeHeap();
            doc["min_heap"] = MemoryManager::getInstance().getMinFreeHeap();
            doc["largest_block"] = ESP.getMaxAllocHeap();
            doc["fragmentation"] = serialized(String(MemoryManager::getInstance().getFragmentationPercent(), 1));
            doc["uptime_s"] = millis() / 1000;
            char buf[192];
            size_t len = serializeJson(doc, buf, sizeof(buf));
            request->send(200, "application/json", String(buf, len));
        });

        // ==========================================
        // MODULE STATUS API
        // ==========================================
        server.on("/api/modules", HTTP_GET, [](AsyncWebServerRequest *request){
            extern String getModuleStatusJson();
            request->send(200, "application/json", getModuleStatusJson());
        });

        // ==========================================
        // WEBSOCKET
        // ==========================================
        ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, 
                          AwsEventType type, void *arg, uint8_t *data, size_t len){
            if (type == WS_EVT_CONNECT) {
                Serial.printf("[WS] Client #%u connected\n", client->id());
            } else if (type == WS_EVT_DISCONNECT) {
                Serial.printf("[WS] Client #%u disconnected\n", client->id());
            }
        });

        server.addHandler(&ws);
    }

    void begin() {
        server.begin();
        Serial.println("[WebServer] Started on port 80");
    }

    // ==========================================
    // DATA SETTERS (called from main loop)
    // ==========================================

    void broadcastLiveData(float batt, float rpm, float tps_pct, float temp) {
        lastBatt = batt;
        lastRPM = rpm;
        lastTPS = tps_pct;
        lastTemp = temp;

        if (ws.count() == 0) return;

        StaticJsonDocument<256> doc;
        doc["batt"] = serialized(String(batt, 2));
        doc["rpm"] = serialized(String(rpm, 0));
        doc["tps_pct"] = serialized(String(tps_pct, 1));
        doc["temp"] = serialized(String(temp, 1));
        doc["fuel_rate"] = serialized(String(lastFuelRate, 2));
        doc["health"] = serialized(String(lastHealthScore, 1));
        
        size_t len = serializeJson(doc, JsonBuffers::telemetryBuf, sizeof(JsonBuffers::telemetryBuf));
        ws.textAll(JsonBuffers::telemetryBuf, len);
        ws.cleanupClients();
    }

    void setHealthData(float score, const String& json) {
        lastHealthScore = score;
        healthJson = json;
    }

    void setFuelData(float rate, float total) {
        lastFuelRate = rate;
        lastFuelTotal = total;
    }

    void setTPSStatus(const String& json) {
        portENTER_CRITICAL(&webDataMux);
        tpsStatusJson = json;
        portEXIT_CRITICAL(&webDataMux);
    }

    void setDTCData(const String& json) { portENTER_CRITICAL(&webDataMux); dtcJson = json; portEXIT_CRITICAL(&webDataMux); }
    void setFreezeData(const String& json) { portENTER_CRITICAL(&webDataMux); freezeJson = json; portEXIT_CRITICAL(&webDataMux); }
    void setTrendData(const String& json) { portENTER_CRITICAL(&webDataMux); trendJson = json; portEXIT_CRITICAL(&webDataMux); }
    void setCrankData(const String& json) { portENTER_CRITICAL(&webDataMux); crankJson = json; portEXIT_CRITICAL(&webDataMux); }
    void setWizardSymptoms(const String& json) { portENTER_CRITICAL(&webDataMux); wizardSymptomsJson = json; portEXIT_CRITICAL(&webDataMux); }
    void setWizardResult(const String& json) { portENTER_CRITICAL(&webDataMux); wizardResultJson = json; portEXIT_CRITICAL(&webDataMux); }
    void setProfileInfo(const String& json) { portENTER_CRITICAL(&webDataMux); profileJson = json; portEXIT_CRITICAL(&webDataMux); }
    void setLastReport(const String& filename) { portENTER_CRITICAL(&webDataMux); lastReportFile = filename; portEXIT_CRITICAL(&webDataMux); }

    // ==========================================
    // REQUEST FLAGS (polled from main loop)
    // ==========================================

    bool isReportRequested() { 
        bool val = reportRequested; 
        reportRequested = false; 
        return val; 
    }

    bool isDTCClearRequested() {
        bool val = dtcClearRequested;
        dtcClearRequested = false;
        return val;
    }

    bool isManualFreezeRequested() {
        bool val = manualFreezeRequested;
        manualFreezeRequested = false;
        return val;
    }

    bool isWizardAnalyzeRequested() {
        bool val = wizardAnalyzeRequested;
        wizardAnalyzeRequested = false;
        return val;
    }

    int getWizardSymptomId() const { return wizardSymptomId; }

    bool isFuelResetRequested() {
        bool val = fuelResetRequested;
        fuelResetRequested = false;
        return val;
    }

    bool isProfileChangeRequested() {
        bool val = profileChangeRequested;
        profileChangeRequested = false;
        return val;
    }

    bool isTPSCalibrateZeroRequested() {
        bool val = tpsCalibrateZeroRequested;
        tpsCalibrateZeroRequested = false;
        return val;
    }

    bool isTPSCalibrateWotRequested() {
        bool val = tpsCalibrateWotRequested;
        tpsCalibrateWotRequested = false;
        return val;
    }

    bool isTPSResetRequested() {
        bool val = tpsResetRequested;
        tpsResetRequested = false;
        return val;
    }

    String getPendingProfile() const { return pendingProfile; }

    AsyncWebServer& getServer() { return server; }

private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    
    // Cached live data
    float lastBatt = 0;
    float lastRPM = 0;
    float lastTPS = 0;
    float lastTemp = 0;
    float lastHealthScore = 100.0f;
    float lastFuelRate = 0;
    float lastFuelTotal = 0;

    // JSON data caches
    String healthJson = "{}";
    String dtcJson = "[]";
    String freezeJson = "[]";
    String trendJson = "{}";
    String crankJson = "[]";
    String wizardSymptomsJson = "[]";
    String wizardResultJson = "{}";
    String profileJson = "{}";
    String lastReportFile = "";
    String tpsStatusJson = "{}";

    // Request flags (volatile for ISR safety on ESP32)
    volatile bool reportRequested = false;
    volatile bool dtcClearRequested = false;
    volatile bool manualFreezeRequested = false;
    volatile bool wizardAnalyzeRequested = false;
    volatile bool fuelResetRequested = false;
    volatile bool profileChangeRequested = false;
    volatile bool tpsCalibrateZeroRequested = false;
    volatile bool tpsCalibrateWotRequested = false;
    volatile bool tpsResetRequested = false;
    int wizardSymptomId = 0;
    String pendingProfile = "";
};

#endif // WEB_SERVER_SERVICE_H
