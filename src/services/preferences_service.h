/**
 * @file preferences_service.h
 * @brief Non-Volatile Storage service for persisting settings across reboots.
 * Stores: last selected profile, running hours, maintenance counters.
 */

#ifndef PREFERENCES_SERVICE_H
#define PREFERENCES_SERVICE_H

#include <Arduino.h>
#include <Preferences.h>

class PreferencesService {
public:
    static PreferencesService& getInstance() {
        static PreferencesService instance;
        return instance;
    }

    void init() {
        prefs.begin("smartfi", false); // RW mode
        Serial.println("[NVS] Preferences initialized");
    }

    // ==========================================
    // LAST PROFILE
    // ==========================================
    void saveLastProfile(const String& path) {
        prefs.putString("last_profile", path);
    }

    String getLastProfile() {
        return prefs.getString("last_profile", "");
    }

    // ==========================================
    // RUNNING HOURS (in minutes, for maintenance tracking)
    // ==========================================
    void addRunningMinutes(uint32_t minutes) {
        uint32_t total = prefs.getUInt("run_minutes", 0);
        total += minutes;
        prefs.putUInt("run_minutes", total);
    }

    uint32_t getRunningMinutes() {
        return prefs.getUInt("run_minutes", 0);
    }

    float getRunningHours() {
        return getRunningMinutes() / 60.0f;
    }

    void resetRunningHours() {
        prefs.putUInt("run_minutes", 0);
    }

    // ==========================================
    // MAINTENANCE COUNTERS
    // ==========================================
    void resetOilChange() {
        prefs.putUInt("oil_at_hrs", getRunningMinutes());
    }

    uint32_t getMinutesSinceOilChange() {
        uint32_t oilAt = prefs.getUInt("oil_at_hrs", 0);
        return getRunningMinutes() - oilAt;
    }

    void resetSparkPlug() {
        prefs.putUInt("plug_at_hrs", getRunningMinutes());
    }

    uint32_t getMinutesSinceSparkPlug() {
        uint32_t plugAt = prefs.getUInt("plug_at_hrs", 0);
        return getRunningMinutes() - plugAt;
    }

    void resetAirFilter() {
        prefs.putUInt("filter_at_hrs", getRunningMinutes());
    }

    uint32_t getMinutesSinceAirFilter() {
        uint32_t filterAt = prefs.getUInt("filter_at_hrs", 0);
        return getRunningMinutes() - filterAt;
    }

    // ==========================================
    // MULTI-DEVICE: per-vehicle session tracking
    // ==========================================
    void saveVehicleSession(const String& vehicleId, uint32_t totalMinutes) {
        String key = "v_" + vehicleId.substring(0, 12); // NVS key max 15 chars
        prefs.putUInt(key.c_str(), totalMinutes);
    }

    uint32_t getVehicleSession(const String& vehicleId) {
        String key = "v_" + vehicleId.substring(0, 12);
        return prefs.getUInt(key.c_str(), 0);
    }

private:
    PreferencesService() {}
    Preferences prefs;
};

#endif // PREFERENCES_SERVICE_H
