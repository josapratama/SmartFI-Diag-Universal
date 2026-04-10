/**
 * @file temperature_service.h
 * @brief Engine Temperature Monitoring Service.
 */

#ifndef TEMPERATURE_SERVICE_H
#define TEMPERATURE_SERVICE_H

#include <memory>
#include "core/channel_manager.h"
#include "config_types.h"

class TemperatureService {
public:
    TemperatureService() : tempChan(nullptr), currentTemp(0) {}

    void init() {
        tempChan = ChannelManager::getInstance().getAnalog("CH_TEMP");
    }

    void update(const VehicleProfile& profile) {
        if (!tempChan || !profile.caps.has_temp) return;
        
        float voltage = tempChan->read();
        // Simplified conversion: assume linear mapping or use a lookup table/Steinhart-Hart in real world
        // For demonstration, map voltage range to temperature range
        float range_v = profile.thresholds.temp_v_max - profile.thresholds.temp_v_min;
        if (range_v != 0) {
            currentTemp = 20.0f + (voltage - profile.thresholds.temp_v_min) * (100.0f / range_v);
        }
    }

    float getTemperature() const { return currentTemp; }
    bool isOverheat(const VehicleProfile& profile) const {
        if (!tempChan) return false;
        return tempChan->read() > profile.thresholds.temp_v_overheat;
    }

private:
    std::shared_ptr<AnalogDriver> tempChan;
    float currentTemp;
};

#endif // TEMPERATURE_SERVICE_H
