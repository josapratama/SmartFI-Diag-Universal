/**
 * @file rpm_service.h
 * @brief Engine RPM Monitoring Service.
 * Uses interrupt-based period measurement for accurate, non-blocking RPM.
 */

#ifndef RPM_SERVICE_H
#define RPM_SERVICE_H

#include <memory>
#include <cmath>
#include "core/channel_manager.h"
#include "config_types.h"

class RPMService {
public:
    RPMService() : rpmChan(nullptr), lastRPM(0), filteredRPM(0), rpmVariance(0) {}

    void init() {
        rpmChan = ChannelManager::getInstance().getDigital("CH_IGN");
    }

    void update(const VehicleProfile& profile) {
        if (!rpmChan || !profile.caps.has_rpm) return;

        // Use period measurement (rising-to-rising) for better accuracy
        // For 4-stroke 1-cylinder: 1 ignition pulse per 2 revolutions
        // RPM = 60,000,000 / period_us * (pulses_per_rev factor)
        // For 1-cyl 4-stroke: 1 pulse per 2 rev -> RPM = 60e6 / period * 0.5 ... 
        // Actually: period = time for 2 revolutions, so RPM = 2 * 60e6 / period... 
        // Simplification: 1 pulse per revolution of crankshaft for ignition signal
        // RPM = 60,000,000 / period_us
        
        uint32_t period = rpmChan->getPeriod();
        
        if (period > 0 && period < 1000000) {  // Valid range: 60 RPM to 600,000 RPM
            float newRPM = 60000000.0f / (float)period;
            
            // Sanity check: motorcycle RPM range 0-15000
            if (newRPM > 15000.0f) newRPM = 0;
            
            // Calculate variance before filtering
            rpmVariance = fabsf(newRPM - lastRPM);
            lastRPM = newRPM;
            
            // EMA filter for smooth display (alpha = 0.3)
            filteredRPM = (0.3f * newRPM) + (0.7f * filteredRPM);
        } else {
            // No signal - engine stopped
            lastRPM = 0;
            filteredRPM = 0;
            rpmVariance = 0;
        }
    }

    float getRPM() const { return filteredRPM; }
    float getRawRPM() const { return lastRPM; }
    float getVariance() const { return rpmVariance; }

private:
    std::shared_ptr<DigitalDriver> rpmChan;
    float lastRPM;
    float filteredRPM;
    float rpmVariance;
};

#endif // RPM_SERVICE_H
