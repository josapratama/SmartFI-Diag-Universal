/**
 * @file session_manager.h
 * @brief Multi-Device Session Manager - tracks diagnostic history per vehicle.
 * Stores session data on SD card per vehicle model/chassis.
 */

#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include "config_types.h"

struct VehicleSession {
    String vehicleId;       // model name or chassis code
    uint32_t totalSessions;
    uint32_t totalMinutes;
    uint32_t lastSessionTime; // millis at last session start
    String lastDTC;         // Last DTC code found
};

class SessionManager {
public:
    SessionManager() : sessionActive(false), sessionStartTime(0) {}

    void startSession(const VehicleProfile& profile) {
        currentVehicleId = profile.model.isEmpty() ? profile.name : profile.model;
        sessionStartTime = millis();
        sessionActive = true;

        // Load existing session data
        loadSession(currentVehicleId);
        currentSession.totalSessions++;
        
        Serial.printf("[Session] Started for: %s (session #%d)\n", 
            currentVehicleId.c_str(), currentSession.totalSessions);
    }

    void update() {
        if (!sessionActive) return;
        // Track running time
        currentSession.lastSessionTime = millis() - sessionStartTime;
    }

    void recordDTC(const String& dtcCode) {
        if (!sessionActive) return;
        currentSession.lastDTC = dtcCode;
    }

    void endSession() {
        if (!sessionActive) return;
        
        uint32_t duration = (millis() - sessionStartTime) / 60000; // minutes
        currentSession.totalMinutes += duration;
        
        // Save to SD
        saveSession(currentVehicleId);
        sessionActive = false;
        
        Serial.printf("[Session] Ended: %s (%d min this session, %d min total)\n",
            currentVehicleId.c_str(), duration, currentSession.totalMinutes);
    }

    VehicleSession getCurrentSession() const { return currentSession; }
    bool isActive() const { return sessionActive; }

    // List all vehicle sessions from SD
    String listSessionsJson() {
        StaticJsonDocument<1024> doc;
        JsonArray arr = doc.to<JsonArray>();

        File dir = SD.open("/sessions");
        if (dir && dir.isDirectory()) {
            File entry;
            while ((entry = dir.openNextFile())) {
                String name = entry.name();
                if (name.endsWith(".json")) {
                    // Read basic info
                    StaticJsonDocument<256> sessionDoc;
                    deserializeJson(sessionDoc, entry);
                    
                    JsonObject obj = arr.createNestedObject();
                    obj["id"] = sessionDoc["id"] | "unknown";
                    obj["sessions"] = sessionDoc["total_sessions"] | 0;
                    obj["hours"] = serialized(String((sessionDoc["total_minutes"].as<uint32_t>() | 0) / 60.0f, 1));
                    obj["last_dtc"] = sessionDoc["last_dtc"] | "";
                }
                entry.close();
            }
            dir.close();
        }

        String output;
        serializeJson(doc, output);
        return output;
    }

private:
    VehicleSession currentSession;
    String currentVehicleId;
    bool sessionActive;
    uint32_t sessionStartTime;

    void loadSession(const String& vehicleId) {
        String path = "/sessions/" + vehicleId + ".json";
        File file = SD.open(path);
        if (file) {
            StaticJsonDocument<256> doc;
            deserializeJson(doc, file);
            currentSession.vehicleId = vehicleId;
            currentSession.totalSessions = doc["total_sessions"] | 0;
            currentSession.totalMinutes = doc["total_minutes"] | 0;
            currentSession.lastDTC = doc["last_dtc"] | "";
            file.close();
        } else {
            currentSession = {vehicleId, 0, 0, 0, ""};
        }
    }

    void saveSession(const String& vehicleId) {
        if (!SD.exists("/sessions")) SD.mkdir("/sessions");
        
        String path = "/sessions/" + vehicleId + ".json";
        File file = SD.open(path, FILE_WRITE);
        if (file) {
            StaticJsonDocument<256> doc;
            doc["id"] = vehicleId;
            doc["total_sessions"] = currentSession.totalSessions;
            doc["total_minutes"] = currentSession.totalMinutes;
            doc["last_dtc"] = currentSession.lastDTC;
            serializeJson(doc, file);
            file.close();
        }
    }
};

#endif // SESSION_MANAGER_H
