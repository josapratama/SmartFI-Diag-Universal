/**
 * @file driver_base.h
 * @brief Base Driver classes for Analog and Digital inputs.
 */

#ifndef DRIVER_BASE_H
#define DRIVER_BASE_H

#include <Arduino.h>
#include "config_types.h"

class AnalogDriver {
public:
    AnalogDriver(const AnalogChannelConfig& config) : cfg(config), filteredValue(0) {}
    
    void init() {
        pinMode(cfg.pin, INPUT);
    }

    float readRawVoltage() {
        int raw = analogRead(cfg.pin);
        // Standard ESP32 3.3V / 12-bit ADC
        float voltage = (raw * 3.3f / 4095.0f);
        return voltage;
    }

    float read() {
        float voltage = readRawVoltage() / cfg.divider_ratio;
        
        // EMA Filtering
        filteredValue = (cfg.filter_alpha * voltage) + ((1.0f - cfg.filter_alpha) * filteredValue);
        return filteredValue;
    }

    float readConverted() {
        float value = read();
        // Simple formula evaluation (can be more complex with a real parser)
        // For now, support "x", "x*C", "x+C"
        if (cfg.formula.indexOf("*") != -1) {
            float factor = cfg.formula.substring(cfg.formula.indexOf("*") + 1).toFloat();
            return value * factor;
        }
        return value;
    }

    const AnalogChannelConfig& getConfig() const { return cfg; }
    String getUnits() const { return cfg.units; }

private:
    AnalogChannelConfig cfg;
    float filteredValue;
};

class DigitalDriver {
public:
    DigitalDriver(const DigitalChannelConfig& config) : cfg(config), lastState(false), lastPulseTime(0) {}
    
    void init() {
        pinMode(cfg.pin, cfg.active_low ? INPUT_PULLUP : INPUT_PULLDOWN);
    }

    bool read() {
        bool state = digitalRead(cfg.pin);
        return cfg.active_low ? !state : state;
    }

    // Pulse measurement for RPM, Injector, etc.
    uint32_t getPulseDuration() {
        // High-speed pulse measurement logic (e.g. using interrupts or pulseIn)
        return pulseIn(cfg.pin, cfg.active_low ? LOW : HIGH, 50000); // 50ms timeout
    }

    // Frequency measurement
    float getFrequency() {
        uint32_t duration = getPulseDuration();
        if (duration == 0) return 0;
        return 1000000.0f / (duration * 2.0f); // Approximate if pulseIn is half-cycle
    }

    const DigitalChannelConfig& getConfig() const { return cfg; }

private:
    DigitalChannelConfig cfg;
    bool lastState;
    uint32_t lastPulseTime;
};

#endif // DRIVER_BASE_H
