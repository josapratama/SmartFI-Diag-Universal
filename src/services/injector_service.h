/**
 * @file injector_service.h
 * @brief Injector Pulse Monitoring Service.
 */

#ifndef INJECTOR_SERVICE_H
#define INJECTOR_SERVICE_H

#include <memory>
#include "core/channel_manager.h"
#include "config_types.h"

class InjectorService {
public:
    InjectorService() : injChan(nullptr), pulseWidth_us(0), dutyCycle(0) {}

    void init() {
        injChan = ChannelManager::getInstance().getDigital("CH_INJ");
    }

    void update(const VehicleProfile& profile) {
        if (!injChan || !profile.caps.has_injector_pulse) return;

        // Measure Injector pulse width (Active Low usually)
        pulseWidth_us = injChan->getPulseDuration();
        
        // Calculate Duty Cycle if RPM is available
        // Need RPM from RPMService or pass it in
    }

    uint32_t getPulseWidth() const { return pulseWidth_us; }
    float getDutyCycle() const { return dutyCycle; }

private:
    std::shared_ptr<DigitalDriver> injChan;
    uint32_t pulseWidth_us;
    float dutyCycle;
};

#endif // INJECTOR_SERVICE_H
