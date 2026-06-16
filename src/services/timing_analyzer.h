/**
 * @file timing_analyzer.h
 * @brief Ignition Timing Analyzer - calculates advance/retard from pulse timing.
 */

#ifndef TIMING_ANALYZER_H
#define TIMING_ANALYZER_H

#include <Arduino.h>
#include <vector>

struct TimingPoint {
    float rpm;
    float advance_deg;  // Estimated advance in degrees
};

class TimingAnalyzer {
public:
    TimingAnalyzer() : lastIgnTime(0), lastInjTime(0), currentAdvance(0) {}

    /**
     * Update timing calculation.
     * Uses the time difference between injector pulse and ignition pulse
     * to estimate ignition advance relative to TDC.
     * 
     * @param ignPeriod_us Period of ignition signal (rising-to-rising) in microseconds
     * @param ignPulse_us Pulse width of ignition signal
     * @param rpm Current RPM
     */
    void update(uint32_t ignPeriod_us, uint32_t ignPulse_us, float rpm) {
        if (rpm < 500 || ignPeriod_us == 0) {
            currentAdvance = 0;
            return;
        }

        // Calculate degrees per microsecond at current RPM
        // 360 degrees per revolution, RPM revolutions per minute
        // degrees_per_us = (RPM * 360) / (60 * 1000000)
        float degreesPerUs = (rpm * 360.0f) / 60000000.0f;

        // Ignition pulse width represents dwell time
        // The falling edge of ignition = spark event
        // Advance can be estimated from pulse position within the period
        float dwellDegrees = ignPulse_us * degreesPerUs;
        
        // Simplified: advance = proportion of period before TDC
        // This is a rough estimate - real measurement needs TDC reference
        currentAdvance = dwellDegrees;

        // Clamp to reasonable range
        if (currentAdvance > 45.0f) currentAdvance = 45.0f;
        if (currentAdvance < 0.0f) currentAdvance = 0.0f;

        // Record timing curve point
        if (rpm > 1000 && points.size() < MAX_POINTS) {
            // Only record at new RPM ranges (every 500 RPM)
            int rpmBucket = (int)(rpm / 500) * 500;
            bool exists = false;
            for (const auto& p : points) {
                if ((int)(p.rpm / 500) * 500 == rpmBucket) { exists = true; break; }
            }
            if (!exists) {
                points.push_back({rpm, currentAdvance});
            }
        }
    }

    float getAdvance() const { return currentAdvance; }
    const std::vector<TimingPoint>& getCurve() const { return points; }

    void clearCurve() { points.clear(); }

    // Check if timing is within expected range for given RPM
    bool isTimingNormal(float rpm) const {
        // Typical motorcycle ignition advance:
        // Idle (1500 RPM): 8-12 degrees
        // Mid (4000 RPM): 20-30 degrees
        // High (8000 RPM): 30-38 degrees
        float expectedMin, expectedMax;
        if (rpm < 2000) { expectedMin = 5; expectedMax = 15; }
        else if (rpm < 5000) { expectedMin = 15; expectedMax = 32; }
        else { expectedMin = 25; expectedMax = 40; }

        return (currentAdvance >= expectedMin && currentAdvance <= expectedMax);
    }

private:
    uint32_t lastIgnTime;
    uint32_t lastInjTime;
    float currentAdvance;
    std::vector<TimingPoint> points;
    static const size_t MAX_POINTS = 20;
};

#endif // TIMING_ANALYZER_H
