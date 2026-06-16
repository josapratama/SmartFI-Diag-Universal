/**
 * @file fuel_estimator.h
 * @brief Fuel Consumption Estimator from injector pulse width and RPM.
 */

#ifndef FUEL_ESTIMATOR_H
#define FUEL_ESTIMATOR_H

#include <Arduino.h>

class FuelEstimator {
public:
    FuelEstimator() : currentRate_ccPerMin(0), totalCC(0), lastUpdateTime(0),
                      avgRate(0), sampleCount(0) {}

    void init() {
        lastUpdateTime = millis();
        totalCC = 0;
        currentRate_ccPerMin = 0;
    }

    /**
     * Update fuel estimation.
     * @param injector_pw_us Injector pulse width in microseconds
     * @param rpm Current engine RPM
     * 
     * Formula for 4-stroke single cylinder:
     * Fuel flow (cc/min) = (INJ_PW_ms * RPM * injector_flow_rate) / (2 * 60 * 1000)
     * 
     * Simplified: For typical 12-hole injector at 3 bar:
     * Flow rate ~80 cc/min at full open
     * Duty cycle = INJ_PW * RPM / (2 * 60 * 1000000) for 4-stroke
     * Fuel cc/min = duty_cycle * flow_rate_cc_per_min
     */
    void update(uint32_t injector_pw_us, float rpm) {
        uint32_t now = millis();
        uint32_t dt = now - lastUpdateTime;
        
        if (dt < 100) return; // Update at max 10Hz
        lastUpdateTime = now;

        if (rpm < 100.0f || injector_pw_us == 0) {
            currentRate_ccPerMin = 0;
            return;  // Don't accumulate when engine is off
        }

        // Calculate duty cycle
        // For 4-stroke 1-cylinder: 1 injection per 2 revolutions
        // Injections per minute = RPM / 2
        float injectionsPerMin = rpm / 2.0f;
        
        // Total injector open time per minute (in seconds)
        float totalOpenTime_s = (injector_pw_us / 1000000.0f) * injectionsPerMin;
        
        // Fuel flow = open_time * flow_rate
        // Typical motorcycle injector: ~80-120 cc/min at full open (3 bar)
        // We use 100 cc/min as default
        currentRate_ccPerMin = totalOpenTime_s * INJECTOR_FLOW_RATE_CC_PER_SEC;

        // Accumulate total (only when engine is running)
        float dt_min = dt / 60000.0f;
        totalCC += currentRate_ccPerMin * dt_min;

        // Running average
        avgRate = ((avgRate * sampleCount) + currentRate_ccPerMin) / (sampleCount + 1);
        if (sampleCount < 10000) sampleCount++;
    }

    float getCurrentRate() const { return currentRate_ccPerMin; }  // cc/min
    float getTotalCC() const { return totalCC; }                    // Total cc consumed
    float getAvgRate() const { return avgRate; }                    // Average cc/min
    
    // Estimated km/L (rough, based on average speed assumption)
    float getEstimatedKmPerLiter(float avgSpeedKmh = 40.0f) const {
        if (avgRate <= 0) return 0;
        // cc/min to L/hour: avgRate * 60 / 1000
        float litersPerHour = avgRate * 60.0f / 1000.0f;
        if (litersPerHour <= 0) return 0;
        // km/L = speed / L_per_hour
        return avgSpeedKmh / litersPerHour;
    }

    void resetTrip() {
        totalCC = 0;
        avgRate = 0;
        sampleCount = 0;
    }

private:
    float currentRate_ccPerMin;
    float totalCC;
    uint32_t lastUpdateTime;
    float avgRate;
    uint32_t sampleCount;

    // Injector flow rate: typical motorcycle injector ~1.67 cc/sec (100 cc/min)
    static constexpr float INJECTOR_FLOW_RATE_CC_PER_SEC = 1.67f;
};

#endif // FUEL_ESTIMATOR_H
