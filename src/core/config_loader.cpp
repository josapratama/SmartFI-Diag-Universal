#include "config_loader.h"
#include <FS.h>
#include <SD.h>

bool ConfigLoader::loadHardwareConfig(const char* filename, HardwareConfig& config) {
    File file = SD.open(filename);
    if (!file) return false;

    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) return false;

    config.project_name = doc["metadata"]["project_name"].as<String>();
    config.hw_revision = doc["metadata"]["hardware_revision"].as<String>();

    // Parse Analog Channels
    JsonArray analogArr = doc["analog_channels"];
    for (JsonObject obj : analogArr) {
        AnalogChannelConfig chan;
        chan.id = obj["id"].as<String>();
        chan.pin = obj["esp32_pin"];
        chan.divider_ratio = obj["divider_ratio"];
        chan.filter_alpha = obj["filter"]["alpha"];
        chan.units = obj["conversion"]["units"].as<String>();
        
        JsonArray bindings = obj["software_binding"]["related_features"];
        for (String b : bindings) chan.software_binding.push_back(b);
        
        config.analog_channels.push_back(chan);
    }

    // Parse Digital Channels
    JsonArray digitalArr = doc["digital_channels"];
    for (JsonObject obj : digitalArr) {
        DigitalChannelConfig chan;
        chan.id = obj["id"].as<String>();
        chan.pin = obj["esp32_pin"];
        chan.active_low = (obj["logic_polarity"].as<String>() == "ACTIVE_LOW");
        chan.debounce_ms = obj["debounce_ms"];
        
        JsonArray bindings = obj["software_binding"]["related_features"];
        for (String b : bindings) chan.software_binding.push_back(b);
        
        config.digital_channels.push_back(chan);
    }

    // UI Configuration
    config.i2c_display.sda = doc["ui_module"]["display"]["sda"];
    config.i2c_display.scl = doc["ui_module"]["display"]["scl"];
    config.i2c_display.address = strtol(doc["ui_module"]["display"]["address"], NULL, 16);

    // Buttons
    JsonArray buttons = doc["ui_module"]["buttons"];
    for (JsonObject btn : buttons) {
        String label = btn["label"].as<String>();
        int pin = btn["pin"];
        if (label == "UP") config.buttons.up = pin;
        else if (label == "DOWN") config.buttons.down = pin;
        else if (label == "SELECT") config.buttons.select = pin;
        else if (label == "BACK") config.buttons.back = pin;
    }

    // Buzzer
    config.buzzer_pin = doc["ui_module"]["audio"]["buzzer_pin"] | 13;

    // Optional Modules
    config.optional = {}; // Zero-init
    if (doc.containsKey("optional_modules")) {
        JsonObject opt = doc["optional_modules"];
        
        if (opt.containsKey("gps")) {
            config.optional.gps.enabled = opt["gps"]["enabled"] | false;
            config.optional.gps.rx_pin = opt["gps"]["rx_pin"] | -1;
            config.optional.gps.tx_pin = opt["gps"]["tx_pin"] | -1;
            config.optional.gps.baud = opt["gps"]["baud"] | 9600;
        }
        if (opt.containsKey("canbus")) {
            config.optional.canbus.enabled = opt["canbus"]["enabled"] | false;
            config.optional.canbus.cs_pin = opt["canbus"]["cs_pin"] | -1;
            config.optional.canbus.speed = opt["canbus"]["speed"] | 500000;
        }
        if (opt.containsKey("relay")) {
            config.optional.relay.enabled = opt["relay"]["enabled"] | false;
            config.optional.relay.pin_1 = opt["relay"]["pin_1"] | -1;
            config.optional.relay.pin_2 = opt["relay"]["pin_2"] | -1;
        }
        if (opt.containsKey("o2_sensor")) {
            config.optional.o2_sensor.enabled = opt["o2_sensor"]["enabled"] | false;
            config.optional.o2_sensor.adc_pin = opt["o2_sensor"]["adc_pin"] | -1;
        }
    }

    return true;
}

bool ConfigLoader::loadVehicleProfile(const char* filename, VehicleProfile& profile) {
    File file = SD.open(filename);
    if (!file) {
        // Fallback: try relative path within /configs/vehicles/
        String path = filename;
        if (!path.startsWith("/")) path = "/configs/vehicles/" + path;
        file = SD.open(path);
        if (!file) return false;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    if (error) return false;

    // 1. Check for parent profile
    if (doc.containsKey("parent_profile")) {
        String parentName = doc["parent_profile"].as<String>();
        String parentPath = "/configs/vehicles/" + parentName + ".json";
        if (!loadVehicleProfile(parentPath.c_str(), profile)) {
            Serial.println("Warning: Parent profile not found: " + parentName);
        }
    }

    // 2. Merge current doc into profile
    mergeProfile(profile, doc.as<JsonObject>());
    return true;
}

void ConfigLoader::mergeProfile(VehicleProfile& p, const JsonObject& obj) {
    if (obj.containsKey("id")) p.id = obj["id"];
    if (obj.containsKey("name")) p.name = obj["name"].as<String>();
    if (obj.containsKey("brand")) p.brand = obj["brand"].as<String>();
    if (obj.containsKey("family")) p.family = obj["family"].as<String>();
    if (obj.containsKey("model")) p.model = obj["model"].as<String>();
    if (obj.containsKey("tier")) p.tier = (CompatibilityTier)obj["tier"].as<int>();

    // Capabilities
    if (obj.containsKey("capabilities")) {
        JsonObject caps = obj["capabilities"];
        if (caps.containsKey("has_tps")) p.caps.has_tps = caps["has_tps"];
        if (caps.containsKey("has_rpm")) p.caps.has_rpm = caps["has_rpm"];
        if (caps.containsKey("has_temp")) p.caps.has_temp = caps["has_temp"];
        if (caps.containsKey("has_mil")) p.caps.has_mil = caps["has_mil"];
        if (caps.containsKey("has_starter_signal")) p.caps.has_starter_signal = caps["has_starter_signal"];
        if (caps.containsKey("has_injector_pulse")) p.caps.has_injector_pulse = caps["has_injector_pulse"];
        if (caps.containsKey("has_ignition_pulse")) p.caps.has_ignition_pulse = caps["has_ignition_pulse"];
        if (caps.containsKey("has_ecu_protocol")) p.caps.has_ecu_protocol = caps["has_ecu_protocol"];
        if (caps.containsKey("has_battery_monitor")) p.caps.has_battery_monitor = caps["has_battery_monitor"];
        if (caps.containsKey("has_fuel_pump_monitor")) p.caps.has_fuel_pump_monitor = caps["has_fuel_pump_monitor"];
    }

    // Thresholds
    if (obj.containsKey("thresholds")) {
        JsonObject t = obj["thresholds"];
        if (t.containsKey("rpm_idle")) {
            p.thresholds.rpm_idle_min = t["rpm_idle"][0];
            p.thresholds.rpm_idle_max = t["rpm_idle"][1];
        }
        if (t.containsKey("tps_v")) {
            p.thresholds.tps_v_idle = t["tps_v"][0];
            p.thresholds.tps_v_full = t["tps_v"][1];
        }
        if (t.containsKey("temp_v")) {
            p.thresholds.temp_v_min = t["temp_v"][0];
            p.thresholds.temp_v_max = t["temp_v"][1];
        }
        if (t.containsKey("temp_v_overheat")) p.thresholds.temp_v_overheat = t["temp_v_overheat"];
        if (t.containsKey("batt_v_idle")) {
            p.thresholds.batt_v_idle_min = t["batt_v_idle"][0];
            p.thresholds.batt_v_idle_max = t["batt_v_idle"][1];
        }
    }
    
    if (obj.containsKey("mil_decoding_mode")) p.mil_mode = obj["mil_decoding_mode"].as<String>();
    
    if (obj.containsKey("startup_thresholds")) {
        JsonObject startup = obj["startup_thresholds"];
        if (startup.containsKey("batt_crank_min")) p.startup.batt_crank_min = startup["batt_crank_min"];
        if (startup.containsKey("crank_duration_max")) p.startup.crank_duration_max_ms = startup["crank_duration_max"];
    }

    if (obj.containsKey("brand_extra")) {
        serializeJson(obj["brand_extra"], p.brand_extra_json);
    }
}
