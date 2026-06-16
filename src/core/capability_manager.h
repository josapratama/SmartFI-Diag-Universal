/**
 * @file capability_manager.h
 * @brief Logic for feature gating based on vehicle capabilities and hardware.
 */

#ifndef CAPABILITY_MANAGER_H
#define CAPABILITY_MANAGER_H

#include "config_types.h"

class CapabilityManager {
public:
    static bool isFeatureAvailable(const String& feature, const VehicleProfile& profile) {
        if (feature == "battery_monitor") return true; // Universal
        if (feature == "tps_monitor") return profile.caps.has_tps;
        if (feature == "rpm_monitor") return profile.caps.has_rpm;
        if (feature == "temp_monitor") return profile.caps.has_temp;
        if (feature == "mil_reader") return profile.caps.has_mil;
        if (feature == "starter_analyzer") return profile.caps.has_starter_signal && profile.caps.has_rpm;
        if (feature == "injector_pulse") return profile.caps.has_injector_pulse;
        if (feature == "ecu_diag") return profile.tier >= TIER_4_FULL_PROTOCOL && profile.caps.has_ecu_protocol;
        
        return false;
    }

    static String getTierName(CompatibilityTier tier) {
        switch(tier) {
            case TIER_1_UNIVERSAL: return "Tier 1: Universal Passive";
            case TIER_2_SEMI_UNIVERSAL: return "Tier 2: Semi-Universal";
            case TIER_3_BRAND_SPECIFIC: return "Tier 3: Brand Specific";
            case TIER_4_FULL_PROTOCOL: return "Tier 4: ECU Protocol";
            default: return "Unknown";
        }
    }
};

#endif // CAPABILITY_MANAGER_H
