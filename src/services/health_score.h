/**
 * @file health_score.h
 * @brief Health Score Calculator - overall motorcycle health percentage.
 */

#ifndef HEALTH_SCORE_H
#define HEALTH_SCORE_H

#include <Arduino.h>
#include "config_types.h"

struct HealthInput {
    float battery_v;
    float tps_v;
    float rpm;
    float temperature;
    float rpm_variance;
    bool mil_active;
    bool tps_out_of_range;
    bool idle_unstable;
    bool charging_failure;
    uint32_t injector_pw_us;
};

struct HealthReport {
    float overall_score;     // 0-100
    float battery_score;     // 0-100
    float tps_score;         // 0-100
    float idle_score;        // 0-100
    float charging_score;    // 0-100
    float thermal_score;     // 0-100
    String summary;          // Human-readable summary
};

class HealthScoreCalculator {
public:
    HealthScoreCalculator() {}

    HealthReport calculate(const VehicleProfile& profile, const HealthInput& input) {
        HealthReport report;

        // Battery Score (weight: 25%)
        report.battery_score = calcBatteryScore(profile, input.battery_v);

        // TPS Score (weight: 20%)
        report.tps_score = calcTPSScore(profile, input.tps_v, input.tps_out_of_range);

        // Idle Stability Score (weight: 20%)
        report.idle_score = calcIdleScore(profile, input.rpm, input.rpm_variance, input.idle_unstable);

        // Charging Score (weight: 20%)
        report.charging_score = calcChargingScore(input.battery_v, input.rpm, input.charging_failure);

        // Thermal Score (weight: 15%)
        report.thermal_score = calcThermalScore(input.temperature);

        // Weighted overall
        report.overall_score = 
            (report.battery_score * 0.25f) +
            (report.tps_score * 0.20f) +
            (report.idle_score * 0.20f) +
            (report.charging_score * 0.20f) +
            (report.thermal_score * 0.15f);

        // MIL penalty
        if (input.mil_active) {
            report.overall_score *= 0.7f; // 30% penalty if MIL is on
        }

        // Clamp
        if (report.overall_score > 100.0f) report.overall_score = 100.0f;
        if (report.overall_score < 0.0f) report.overall_score = 0.0f;

        // Summary
        report.summary = generateSummary(report);
        return report;
    }

private:
    float calcBatteryScore(const VehicleProfile& profile, float v) {
        if (v < 10.0f) return 0.0f;
        if (v < 11.5f) return 20.0f;
        if (v < profile.thresholds.batt_v_idle_min) return 50.0f;
        if (v <= profile.thresholds.batt_v_idle_max) return 100.0f;
        if (v > 15.0f) return 40.0f; // Overcharging
        return 80.0f;
    }

    float calcTPSScore(const VehicleProfile& profile, float v, bool outOfRange) {
        if (outOfRange) return 10.0f;
        // Check if within expected range
        if (v >= (profile.thresholds.tps_v_idle - 0.1f) && 
            v <= (profile.thresholds.tps_v_full + 0.1f)) {
            return 100.0f;
        }
        return 60.0f;
    }

    float calcIdleScore(const VehicleProfile& profile, float rpm, float variance, bool unstable) {
        if (rpm == 0) return 100.0f; // Engine off, no idle issue
        if (unstable) return 30.0f;
        if (variance > 200.0f) return 40.0f;
        if (variance > 100.0f) return 70.0f;
        if (variance > 50.0f) return 85.0f;
        return 100.0f;
    }

    float calcChargingScore(float battV, float rpm, bool failure) {
        if (rpm < 2000.0f) return 100.0f; // Can't assess charging at low RPM
        if (failure) return 20.0f;
        if (battV >= 13.5f && battV <= 14.8f) return 100.0f;
        if (battV >= 13.0f && battV <= 15.2f) return 70.0f;
        return 40.0f;
    }

    float calcThermalScore(float temp) {
        if (temp < 30.0f) return 100.0f; // Cold engine
        if (temp <= 95.0f) return 100.0f; // Normal operating
        if (temp <= 100.0f) return 70.0f; // Getting hot
        if (temp <= 110.0f) return 40.0f; // Overheat warning
        return 10.0f; // Critical overheat
    }

    String generateSummary(const HealthReport& r) {
        if (r.overall_score >= 85.0f) return "Kondisi motor sangat baik";
        if (r.overall_score >= 70.0f) return "Kondisi baik, perhatikan minor issue";
        if (r.overall_score >= 50.0f) return "Perlu perhatian - ada masalah terdeteksi";
        if (r.overall_score >= 30.0f) return "Kondisi buruk - segera periksa";
        return "KRITIS - jangan digunakan sebelum diperbaiki";
    }
};

#endif // HEALTH_SCORE_H
