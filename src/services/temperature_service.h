/**
 * @file temperature_service.h
 * @brief Engine Temperature Monitoring Service for Automotive NTC Thermistors.
 */

#ifndef TEMPERATURE_SERVICE_H
#define TEMPERATURE_SERVICE_H

#include <memory>
#include <algorithm>
#include "core/channel_manager.h"
#include "config_types.h"

class TemperatureService {
public:
    TemperatureService() : tempChan(nullptr), currentTemp(25.0f), rawVoltage(0.0f) {}

    void init() {
        tempChan = ChannelManager::getInstance().getAnalog("CH_TEMP");
    }

    void update(const VehicleProfile& profile) {
        if (!tempChan || !profile.caps.has_temp) return;
        
        rawVoltage = tempChan->read();
        
        // BUG FIX #5: Automotive ECT/EOT sensors are NTC Thermistors (Negative Temperature Coefficient)
        // High voltage = Cold engine; Low voltage = Hot engine.
        float v_min = profile.thresholds.temp_v_min; // Hot voltage (~0.5V)
        float v_max = profile.thresholds.temp_v_max; // Cold voltage (~3.9V)
        float range_v = v_max - v_min;
        
        if (range_v > 0.1f) {
            // NTC characteristic: As temperature rises from 20°C to 120°C, voltage drops from v_max to v_min
            float calculated = 20.0f + ((v_max - rawVoltage) / range_v) * 100.0f;
            currentTemp = std::max(-10.0f, std::min(150.0f, calculated));
        } else {
            currentTemp = 25.0f;
        }
    }

    float getTemperature() const { 
        return currentTemp; 
    }

    float getRawVoltage() const {
        return rawVoltage;
    }

    bool isOverheat(const VehicleProfile& profile) const {
        if (!tempChan) return false;
        // Engine overheat threshold: standard automotive threshold is >= 105°C
        return currentTemp >= 105.0f;
    }

private:
    std::shared_ptr<AnalogDriver> tempChan;
    float currentTemp;
    float rawVoltage;
};

#endif // TEMPERATURE_SERVICE_H
