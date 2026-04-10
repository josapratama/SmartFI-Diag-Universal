/**
 * @file battery_service.h
 * @brief Battery Monitoring Service for Smart FI Diagnostic Tool.
 */

#ifndef BATTERY_SERVICE_H
#define BATTERY_SERVICE_H

#include <memory>
#include "core/channel_manager.h"
#include "config_types.h"

class BatteryService {
public:
    BatteryService() : batteryChan(nullptr) {}

    void init() {
        // Find channel by ID or software binding
        batteryChan = ChannelManager::getInstance().getAnalog("CH_BATT");
        if (batteryChan) {
            Serial.println("BatteryService: Bound to CH_BATT");
        } else {
            Serial.println("BatteryService: FAILED to bind to CH_BATT");
        }
    }

    void update(const VehicleProfile& profile) {
        if (!batteryChan || !profile.caps.has_battery_monitor) return;

        currentVoltage = batteryChan->read();
        
        // Example: Cranking analysis
        if (currentVoltage < profile.startup.batt_crank_min) {
            // Serial.printf("Battery Warning: Low voltage detected: %.2fV\n", currentVoltage);
        }
    }

    float getVoltage() const { return currentVoltage; }

private:
    std::shared_ptr<AnalogDriver> batteryChan;
    float currentVoltage = 0;
};

#endif // BATTERY_SERVICE_H
