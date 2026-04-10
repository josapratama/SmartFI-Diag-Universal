/**
 * @file config_loader.h
 * @brief Configuration Loader for Smart FI Diagnostic Tool.
 */

#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <ArduinoJson.h>
#include "config_types.h"

class ConfigLoader {
public:
    static bool loadHardwareConfig(const char* filename, HardwareConfig& config);
    static bool loadVehicleProfile(const char* filename, VehicleProfile& profile);

private:
    static bool parseAnalogChannel(const JsonObject& obj, AnalogChannelConfig& chan);
    static bool parseDigitalChannel(const JsonObject& obj, DigitalChannelConfig& chan);
    static void mergeProfile(VehicleProfile& base, const JsonObject& overlay);
};

#endif // CONFIG_LOADER_H
