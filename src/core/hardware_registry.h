/**
 * @file hardware_registry.h
 * @brief Registry for Hardware Configuration.
 */

#ifndef HARDWARE_REGISTRY_H
#define HARDWARE_REGISTRY_H

#include "config_types.h"
#include <vector>

class HardwareRegistry {
public:
    static HardwareRegistry& getInstance() {
        static HardwareRegistry instance;
        return instance;
    }

    void setConfig(const HardwareConfig& config) {
        currentConfig = config;
    }

    const HardwareConfig& getConfig() const {
        return currentConfig;
    }

    const AnalogChannelConfig* getAnalogChannel(const String& id) const {
        for (const auto& chan : currentConfig.analog_channels) {
            if (chan.id == id) return &chan;
        }
        return nullptr;
    }

    const DigitalChannelConfig* getDigitalChannel(const String& id) const {
        for (const auto& chan : currentConfig.digital_channels) {
            if (chan.id == id) return &chan;
        }
        return nullptr;
    }

private:
    HardwareRegistry() {}
    HardwareConfig currentConfig;
};

#endif // HARDWARE_REGISTRY_H
