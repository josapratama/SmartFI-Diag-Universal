/**
 * @file protocol_plugin.h
 * @brief Abstract interface for ECU protocol plug-ins (Tier 4).
 */

#ifndef PROTOCOL_PLUGIN_H
#define PROTOCOL_PLUGIN_H

#include <Arduino.h>
#include <vector>

struct DTC {
    String code;
    String description;
    bool is_active;
};

class ProtocolPlugin {
public:
    virtual ~ProtocolPlugin() {}
    
    // Core ECU Operations
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() = 0;
    
    // Data Retrieval
    virtual std::vector<DTC> readDTCs() = 0;
    virtual bool clearDTCs() = 0;
    virtual float getLiveParam(const String& param_name) = 0;
    
    // Metadata
    virtual String getProtocolName() = 0;
    virtual bool supportsFeature(const String& feature) = 0;
};

#endif // PROTOCOL_PLUGIN_H
