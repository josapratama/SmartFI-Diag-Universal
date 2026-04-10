/**
 * @file channel_manager.h
 * @brief Channel Registry and Manager for Smart FI Diagnostic Tool.
 */

#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H

#include <map>
#include <memory>
#include <vector>
#include "../drivers/driver_base.h"
#include "config_types.h"

class ChannelManager {
public:
    static ChannelManager& getInstance() {
        static ChannelManager instance;
        return instance;
    }

    void init(const HardwareConfig& config) {
        // Clear old channels
        analogDrivers.clear();
        digitalDrivers.clear();

        // Initialize Analog Drivers
        for (const auto& cfg : config.analog_channels) {
            auto driver = std::make_shared<AnalogDriver>(cfg);
            driver->init();
            analogDrivers[cfg.id] = driver;
        }

        // Initialize Digital Drivers
        for (const auto& cfg : config.digital_channels) {
            auto driver = std::make_shared<DigitalDriver>(cfg);
            driver->init();
            digitalDrivers[cfg.id] = driver;
        }
    }

    std::shared_ptr<AnalogDriver> getAnalog(const String& id) {
        if (analogDrivers.find(id) != analogDrivers.end()) return analogDrivers[id];
        return nullptr;
    }

    std::shared_ptr<DigitalDriver> getDigital(const String& id) {
        if (digitalDrivers.find(id) != digitalDrivers.end()) return digitalDrivers[id];
        return nullptr;
    }

    // Get channels by software feature binding
    std::vector<std::shared_ptr<AnalogDriver>> findAnalogByFeature(const String& feature) {
        std::vector<std::shared_ptr<AnalogDriver>> results;
        for (auto const& [id, driver] : analogDrivers) {
            for (const auto& binding : driver->getConfig().software_binding) {
                if (binding == feature) results.push_back(driver);
            }
        }
        return results;
    }

private:
    ChannelManager() {}
    std::map<String, std::shared_ptr<AnalogDriver>> analogDrivers;
    std::map<String, std::shared_ptr<DigitalDriver>> digitalDrivers;
};

#endif // CHANNEL_MANAGER_H
