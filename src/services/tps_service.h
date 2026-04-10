/**
 * @file tps_service.h
 * @brief TPS Monitoring Service.
 */

#ifndef TPS_SERVICE_H
#define TPS_SERVICE_H

#include <memory>
#include "core/channel_manager.h"
#include "config_types.h"

class TPSService {
public:
    TPSService() : tpsChan(nullptr) {}

    void init() {
        tpsChan = ChannelManager::getInstance().getAnalog("CH_TPS");
    }

    void update(const VehicleProfile& profile) {
        if (!tpsChan || !profile.caps.has_tps) return;
        currentVoltage = tpsChan->read();
        
        // Calculate percentage based on profile
        float range = profile.thresholds.tps_v_full - profile.thresholds.tps_v_idle;
        if (range > 0) {
            currentPercent = ((currentVoltage - profile.thresholds.tps_v_idle) / range) * 100.0f;
            if (currentPercent < 0) currentPercent = 0;
            if (currentPercent > 100) currentPercent = 100;
        }
    }

    float getVoltage() const { return currentVoltage; }
    float getPercent() const { return currentPercent; }

private:
    std::shared_ptr<AnalogDriver> tpsChan;
    float currentVoltage = 0;
    float currentPercent = 0;
};

#endif // TPS_SERVICE_H
