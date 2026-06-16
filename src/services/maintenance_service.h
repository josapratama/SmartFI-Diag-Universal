/**
 * @file maintenance_service.h
 * @brief Maintenance Reminder Service - tracks running hours and alerts for service intervals.
 */

#ifndef MAINTENANCE_SERVICE_H
#define MAINTENANCE_SERVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "services/preferences_service.h"

struct MaintenanceItem {
    String name;
    uint32_t interval_hours;  // Service interval in running hours
    uint32_t hours_since;     // Hours since last service
    bool overdue;
    int percent_remaining;    // 0-100, 0 = overdue
};

class MaintenanceService {
public:
    MaintenanceService() : engineRunning(false), lastMinuteUpdate(0) {}

    void init() {
        Serial.printf("[Maintenance] Running hours: %.1f hrs\n", 
            PreferencesService::getInstance().getRunningHours());
    }

    // Call every loop iteration - tracks engine running time
    void update(float rpm) {
        bool wasRunning = engineRunning;
        engineRunning = (rpm > 500.0f);

        // Track running minutes
        if (engineRunning) {
            uint32_t now = millis();
            if (now - lastMinuteUpdate >= 60000) { // Every minute
                PreferencesService::getInstance().addRunningMinutes(1);
                lastMinuteUpdate = now;
            }
        }
    }

    // Get all maintenance items with status
    std::vector<MaintenanceItem> getStatus() {
        std::vector<MaintenanceItem> items;
        auto& prefs = PreferencesService::getInstance();

        // Oil Change: every 30 hours (~2000km at 65km/h avg)
        items.push_back(createItem("Ganti Oli", 30, prefs.getMinutesSinceOilChange() / 60));

        // Spark Plug: every 120 hours (~8000km)
        items.push_back(createItem("Ganti Busi", 120, prefs.getMinutesSinceSparkPlug() / 60));

        // Air Filter: every 60 hours (~4000km)
        items.push_back(createItem("Cek Filter Udara", 60, prefs.getMinutesSinceAirFilter() / 60));

        // Valve Clearance: every 300 hours (~20000km)
        // (using oil change counter as proxy - user should reset manually)
        
        return items;
    }

    // Check if any maintenance is overdue
    bool hasOverdueItems() {
        auto items = getStatus();
        for (const auto& item : items) {
            if (item.overdue) return true;
        }
        return false;
    }

    // Reset specific maintenance counter
    void resetOilChange() { PreferencesService::getInstance().resetOilChange(); }
    void resetSparkPlug() { PreferencesService::getInstance().resetSparkPlug(); }
    void resetAirFilter() { PreferencesService::getInstance().resetAirFilter(); }

    String toJson() {
        StaticJsonDocument<512> doc;
        doc["running_hours"] = serialized(String(PreferencesService::getInstance().getRunningHours(), 1));
        
        JsonArray arr = doc.createNestedArray("items");
        auto items = getStatus();
        for (const auto& item : items) {
            JsonObject obj = arr.createNestedObject();
            obj["name"] = item.name;
            obj["interval_hrs"] = item.interval_hours;
            obj["since_hrs"] = item.hours_since;
            obj["overdue"] = item.overdue;
            obj["remaining_pct"] = item.percent_remaining;
        }

        String output;
        serializeJson(doc, output);
        return output;
    }

private:
    bool engineRunning;
    uint32_t lastMinuteUpdate;

    MaintenanceItem createItem(const String& name, uint32_t intervalHrs, uint32_t sinceHrs) {
        MaintenanceItem item;
        item.name = name;
        item.interval_hours = intervalHrs;
        item.hours_since = sinceHrs;
        item.overdue = (sinceHrs >= intervalHrs);
        item.percent_remaining = max(0, (int)(100 - (sinceHrs * 100 / intervalHrs)));
        return item;
    }
};

#endif // MAINTENANCE_SERVICE_H
