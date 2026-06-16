/**
 * @file config_types.h
 * @brief Global configuration structs for Hardware and Vehicle Profiles.
 */

#ifndef CONFIG_TYPES_H
#define CONFIG_TYPES_H

#include <Arduino.h>
#include <vector>

// ==========================================
// 1. HARDWARE CHANNEL TYPES
// ==========================================
struct AnalogChannelConfig {
    String id;
    int pin;
    float divider_ratio;
    String filter_type;
    float filter_alpha;
    String formula;
    String units;
    std::vector<String> software_binding;
};

struct DigitalChannelConfig {
    String id;
    int pin;
    String interface_type;
    bool active_low;
    int debounce_ms;
    String edge_type;
    std::vector<String> software_binding;
};

struct OptionalModules {
    struct {
        bool enabled;
        int rx_pin;
        int tx_pin;
        int baud;
    } gps;
    struct {
        bool enabled;
        int cs_pin;
        uint32_t speed;
    } canbus;
    struct {
        bool enabled;
        int pin_1;
        int pin_2;
    } relay;
    struct {
        bool enabled;
        int adc_pin;
    } o2_sensor;
};

struct HardwareConfig {
    String project_name;
    String hw_revision;
    std::vector<AnalogChannelConfig> analog_channels;
    std::vector<DigitalChannelConfig> digital_channels;
    struct {
        int sda;
        int scl;
        uint8_t address;
    } i2c_display;
    struct {
        int up, down, select, back;
    } buttons;
    int buzzer_pin;
    OptionalModules optional;
};

// ==========================================
// 2. VEHICLE PROFILE TYPES
// ==========================================

enum CompatibilityTier {
    TIER_1_UNIVERSAL = 1,
    TIER_2_SEMI_UNIVERSAL = 2,
    TIER_3_BRAND_SPECIFIC = 3,
    TIER_4_FULL_PROTOCOL = 4
};

struct Capabilities {
    bool has_tps;
    bool has_rpm;
    bool has_temp;
    bool has_mil;
    bool has_starter_signal;
    bool has_injector_pulse;
    bool has_ignition_pulse;
    bool has_ecu_protocol;
    bool has_battery_monitor;
    bool has_fuel_pump_monitor;
};

struct VehicleProfile {
    int id;
    String name;
    String brand;
    String family;
    String model;
    String parent_profile; // For inheritance (Base -> Brand -> Family -> Model)
    CompatibilityTier tier;
    Capabilities caps;
    
    // Thresholds (Inheritable/Overridable)
    struct {
        float rpm_idle_min;
        float rpm_idle_max;
        float tps_v_idle;
        float tps_v_full;
        float temp_v_min;
        float temp_v_max;
        float temp_v_overheat;
        float batt_v_idle_min;
        float batt_v_idle_max;
    } thresholds;
    
    String mil_mode; // "PULSE_COUNT", "MORSE_CODE", "SERIAL"
    
    struct {
        float batt_crank_min;
        int crank_duration_max_ms;
    } startup;

    // Brand-specific extensions (JSON string for flexibility)
    String brand_extra_json;
};

#endif // CONFIG_TYPES_H
