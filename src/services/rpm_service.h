/**
 * @file rpm_service.h
 * @brief Engine RPM Monitoring Service.
 */

#ifndef RPM_SERVICE_H
#define RPM_SERVICE_H

#include <memory>
#include "core/channel_manager.h"
#include "config_types.h"

class RPMService {
public:
    RPMService() : rpmChan(nullptr), lastRPM(0), rpmVariance(0) {}

    void init() {
        rpmChan = ChannelManager::getInstance().getDigital("CH_IGN");
    }

    void update(const VehicleProfile& profile) {
        if (!rpmChan || !profile.caps.has_rpm) return;

        // Simplified RPM calculation from pulse duration (microseconds)
        // For 4-stroke 1-cylinder: 2 revolutions = 1 pulse
        // RPM = (60,000,000 / pulse_duration_us) * (2 if pulse per 2 rev else 1)
        uint32_t pulseDuration = rpmChan->getPulseDuration();
        if (pulseDuration > 0 && pulseDuration < 1000000) {
            float newRPM = 60000000.0f / pulseDuration;
            rpmVariance = abs(newRPM - lastRPM);
            lastRPM = newRPM;
        } else {
            lastRPM = 0;
            rpmVariance = 0;
        }
    }

    float getRPM() const { return lastRPM; }
    float getVariance() const { return rpmVariance; }

private:
    std::shared_ptr<DigitalDriver> rpmChan;
    float lastRPM;
    float rpmVariance;
};

#endif // RPM_SERVICE_H
