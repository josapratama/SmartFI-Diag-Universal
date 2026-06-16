/**
 * @file dtc_reader.h
 * @brief DTC (Diagnostic Trouble Code) Reader Service.
 * Interfaces with ProtocolManager to read/clear DTCs from ECU.
 */

#ifndef DTC_READER_H
#define DTC_READER_H

#include <Arduino.h>
#include <vector>
#include "config_types.h"
#include "core/protocol_manager.h"
#include "core/protocol_plugin.h"

struct DTCResult {
    bool success;
    std::vector<DTC> codes;
    uint32_t timestamp;
    String protocol_name;
};

class DTCReader {
public:
    DTCReader() : lastResult({false, {}, 0, ""}) {}

    bool isECUReady(const VehicleProfile& profile) {
        if (profile.tier < TIER_4_FULL_PROTOCOL || !profile.caps.has_ecu_protocol) {
            return false;
        }
        auto plugin = ProtocolManager::getInstance().getActivePlugin();
        return (plugin != nullptr && plugin->isConnected());
    }

    DTCResult readDTCs(const VehicleProfile& profile) {
        DTCResult result;
        result.success = false;
        result.timestamp = millis();

        if (!isECUReady(profile)) {
            // Fallback: use MIL blink code if available
            result.protocol_name = "MIL_BLINK";
            return result;
        }

        auto plugin = ProtocolManager::getInstance().getActivePlugin();
        result.protocol_name = plugin->getProtocolName();
        result.codes = plugin->readDTCs();
        result.success = true;

        lastResult = result;
        Serial.printf("[DTCReader] Read %d DTCs via %s\n", 
            result.codes.size(), result.protocol_name.c_str());
        return result;
    }

    bool clearDTCs(const VehicleProfile& profile) {
        if (!isECUReady(profile)) return false;

        auto plugin = ProtocolManager::getInstance().getActivePlugin();
        bool success = plugin->clearDTCs();
        
        if (success) {
            lastResult.codes.clear();
            Serial.println("[DTCReader] DTCs cleared successfully");
        }
        return success;
    }

    DTCResult getLastResult() const { return lastResult; }

private:
    DTCResult lastResult;
};

#endif // DTC_READER_H
