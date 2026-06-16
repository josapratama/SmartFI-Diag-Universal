/**
 * @file module_status.h
 * @brief Module Status Tracker - monitors all optional and core modules.
 * Provides status info for OLED display and web dashboard notifications.
 */

#ifndef MODULE_STATUS_H
#define MODULE_STATUS_H

#include <Arduino.h>
#include <ArduinoJson.h>

enum ModuleState {
    MOD_DISABLED,       // Not enabled in config
    MOD_NOT_DETECTED,   // Enabled but hardware not found
    MOD_CONNECTED,      // Working normally
    MOD_ERROR           // Was connected but lost connection
};

struct ModuleInfo {
    String name;
    ModuleState state;
    String detail;
};

class ModuleStatusTracker {
public:
    static ModuleStatusTracker& getInstance() {
        static ModuleStatusTracker instance;
        return instance;
    }

    void setStatus(const String& moduleName, ModuleState state, const String& detail = "") {
        for (auto& m : modules) {
            if (m.name == moduleName) {
                m.state = state;
                m.detail = detail;
                return;
            }
        }
        // New module
        modules.push_back({moduleName, state, detail});
    }

    ModuleState getStatus(const String& moduleName) const {
        for (const auto& m : modules) {
            if (m.name == moduleName) return m.state;
        }
        return MOD_DISABLED;
    }

    // Check if any module has a problem (not detected or error)
    bool hasWarnings() const {
        for (const auto& m : modules) {
            if (m.state == MOD_NOT_DETECTED || m.state == MOD_ERROR) return true;
        }
        return false;
    }

    // Get count of active (connected) modules
    int getActiveCount() const {
        int count = 0;
        for (const auto& m : modules) {
            if (m.state == MOD_CONNECTED) count++;
        }
        return count;
    }

    // Get notification messages for modules with issues
    String getNotifications() const {
        String msg = "";
        for (const auto& m : modules) {
            if (m.state == MOD_NOT_DETECTED) {
                msg += "! " + m.name + ": tidak terdeteksi";
                if (!m.detail.isEmpty()) msg += " (" + m.detail + ")";
                msg += "\n";
            } else if (m.state == MOD_ERROR) {
                msg += "X " + m.name + ": error";
                if (!m.detail.isEmpty()) msg += " (" + m.detail + ")";
                msg += "\n";
            }
        }
        return msg;
    }

    // JSON for web dashboard
    String toJson() const {
        StaticJsonDocument<1024> doc;
        JsonArray arr = doc.to<JsonArray>();
        
        for (const auto& m : modules) {
            JsonObject obj = arr.createNestedObject();
            obj["name"] = m.name;
            obj["state"] = stateToString(m.state);
            obj["detail"] = m.detail;
        }

        String output;
        serializeJson(doc, output);
        return output;
    }

    // Summary string for OLED
    String getOLEDSummary() const {
        int total = modules.size();
        int active = getActiveCount();
        int disabled = 0;
        int problems = 0;
        
        for (const auto& m : modules) {
            if (m.state == MOD_DISABLED) disabled++;
            if (m.state == MOD_NOT_DETECTED || m.state == MOD_ERROR) problems++;
        }

        String summary = String(active) + "/" + String(total - disabled) + " aktif";
        if (problems > 0) summary += " !" + String(problems) + " masalah";
        return summary;
    }

private:
    ModuleStatusTracker() {}
    std::vector<ModuleInfo> modules;

    static String stateToString(ModuleState state) {
        switch (state) {
            case MOD_DISABLED: return "disabled";
            case MOD_NOT_DETECTED: return "not_detected";
            case MOD_CONNECTED: return "connected";
            case MOD_ERROR: return "error";
            default: return "unknown";
        }
    }
};

#endif // MODULE_STATUS_H
