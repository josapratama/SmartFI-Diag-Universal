/**
 * @file o2_sensor_service.h
 * @brief O2 (Lambda) Sensor Reader - AFR analysis from narrowband O2 sensor.
 * Optional: only active if CH_O2 channel is configured in hardware config.
 */

#ifndef O2_SENSOR_SERVICE_H
#define O2_SENSOR_SERVICE_H

#include <Arduino.h>
#include <memory>
#include "core/channel_manager.h"
#include "config_types.h"

enum AFRCondition {
    AFR_UNKNOWN,
    AFR_RICH,           // O2 voltage > 0.6V (lambda < 1)
    AFR_STOICH,         // O2 voltage ~0.45V (lambda = 1)
    AFR_LEAN,           // O2 voltage < 0.3V (lambda > 1)
    AFR_SENSOR_COLD     // O2 voltage near 0 (sensor not heated yet)
};

class O2SensorService {
public:
    O2SensorService() : o2Chan(nullptr), voltage(0), condition(AFR_UNKNOWN),
                         available(false), switchCount(0), lastSwitchTime(0),
                         lastCondition(AFR_UNKNOWN) {}

    void init() {
        o2Chan = ChannelManager::getInstance().getAnalog("CH_O2");
        available = (o2Chan != nullptr);
        if (available) {
            Serial.println("[O2] Sensor channel found (CH_O2)");
        } else {
            Serial.println("[O2] No CH_O2 channel configured - O2 monitoring disabled");
        }
    }

    bool isAvailable() const { return available; }

    void update() {
        if (!available) return;

        voltage = o2Chan->read();

        // Narrowband O2 sensor interpretation:
        // 0.0 - 0.1V: Sensor cold or disconnected
        // 0.1 - 0.35V: Lean (lambda > 1)
        // 0.35 - 0.55V: Stoichiometric (lambda ≈ 1)
        // 0.55 - 1.0V: Rich (lambda < 1)
        
        AFRCondition newCondition;
        if (voltage < 0.1f) {
            newCondition = AFR_SENSOR_COLD;
        } else if (voltage < 0.35f) {
            newCondition = AFR_LEAN;
        } else if (voltage <= 0.55f) {
            newCondition = AFR_STOICH;
        } else {
            newCondition = AFR_RICH;
        }

        // Count rich/lean switches (healthy sensor switches rapidly at stoich)
        if (newCondition != lastCondition && 
            newCondition != AFR_SENSOR_COLD && lastCondition != AFR_SENSOR_COLD) {
            uint32_t now = millis();
            if (now - lastSwitchTime < 2000) { // Only count rapid switches
                switchCount++;
            }
            lastSwitchTime = now;
        }
        lastCondition = newCondition;
        condition = newCondition;
    }

    float getVoltage() const { return voltage; }
    AFRCondition getCondition() const { return condition; }
    
    String getConditionString() const {
        switch (condition) {
            case AFR_RICH: return "RICH";
            case AFR_STOICH: return "STOICH";
            case AFR_LEAN: return "LEAN";
            case AFR_SENSOR_COLD: return "COLD";
            default: return "N/A";
        }
    }

    // Healthy O2 sensor switches 5-10 times per 10 seconds at idle
    bool isSensorHealthy() const {
        return switchCount > 3; // At least some switching activity
    }

    uint32_t getSwitchCount() const { return switchCount; }
    void resetSwitchCount() { switchCount = 0; }

private:
    std::shared_ptr<AnalogDriver> o2Chan;
    float voltage;
    AFRCondition condition;
    bool available;
    uint32_t switchCount;
    uint32_t lastSwitchTime;
    AFRCondition lastCondition;
};

#endif // O2_SENSOR_SERVICE_H
