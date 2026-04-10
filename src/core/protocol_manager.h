/**
 * @file protocol_manager.h
 * @brief Manager for ECU Protocol Plug-ins.
 */

#ifndef PROTOCOL_MANAGER_H
#define PROTOCOL_MANAGER_H

#include <map>
#include <memory>
#include "protocol_plugin.h"

class ProtocolManager {
public:
    static ProtocolManager& getInstance() {
        static ProtocolManager instance;
        return instance;
    }

    void registerPlugin(const String& brand, std::shared_ptr<ProtocolPlugin> plugin) {
        plugins[brand] = plugin;
    }

    std::shared_ptr<ProtocolPlugin> getPlugin(const String& brand) {
        if (plugins.find(brand) != plugins.end()) return plugins[brand];
        return nullptr;
    }

    void update(const VehicleProfile& profile) {
        if (profile.tier >= TIER_4_FULL_PROTOCOL && profile.caps.has_ecu_protocol) {
            auto plugin = getPlugin(profile.brand);
            if (plugin && !plugin->isConnected()) {
                plugin->connect();
            }
        }
    }

private:
    ProtocolManager() {}
    std::map<String, std::shared_ptr<ProtocolPlugin>> plugins;
};

#endif // PROTOCOL_MANAGER_H
