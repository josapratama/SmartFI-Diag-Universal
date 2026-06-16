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
            if (!plugin) return;

            // Already connected to the right plugin
            if (plugin == activePlugin && plugin->isConnected()) return;

            // Disconnect previous plugin if switching brands
            if (activePlugin && activePlugin != plugin && activePlugin->isConnected()) {
                activePlugin->disconnect();
            }

            // Retry cooldown: don't spam connect attempts
            uint32_t now = millis();
            if (now - lastConnectAttempt < CONNECT_RETRY_INTERVAL_MS) return;
            lastConnectAttempt = now;

            if (plugin->connect()) {
                activePlugin = plugin;
            }
        }
    }

    std::shared_ptr<ProtocolPlugin> getActivePlugin() {
        return activePlugin;
    }

private:
    ProtocolManager() : lastConnectAttempt(0) {}
    std::map<String, std::shared_ptr<ProtocolPlugin>> plugins;
    std::shared_ptr<ProtocolPlugin> activePlugin = nullptr;
    uint32_t lastConnectAttempt;
    static const uint32_t CONNECT_RETRY_INTERVAL_MS = 5000;  // Retry every 5 seconds
};

#endif // PROTOCOL_MANAGER_H
