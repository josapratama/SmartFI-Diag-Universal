/**
 * @file diag_engine.h
 * @brief Rule-based Diagnostic Engine for FI Motorcycle Symptoms.
 */

#ifndef DIAG_ENGINE_H
#define DIAG_ENGINE_H

#include <Arduino.h>
#include "config_types.h"

// ==========================================
// DIAGNOSTIC FLAGS
// ==========================================
typedef struct {
    bool low_battery;
    bool tps_noisy;
    bool tps_out_of_range;
    bool idle_unstable;
    bool charging_failure;
    bool ignition_missing;
    bool injector_missing;
    bool mil_error_detected;
    float health_score; // 0.0 - 100.0
} DiagStatus;

// ==========================================
// SYMPTOM CODES
// ==========================================
enum Symptom {
    SYMPTOM_NONE,
    SYMPTOM_ROUGH_START,
    SYMPTOM_LOSS_OF_IDLE,
    SYMPTOM_STALL_ON_OFF_THROTTLE,
    SYMPTOM_BREBET_ACCEL,
    SYMPTOM_HIGH_CONSUMPTION,
    SYMPTOM_OVERHEAT
};

class DiagEngine {
public:
    DiagEngine();
    void update(const VehicleProfile& profile, float battery_v, float tps_v, float rpm, float temp, bool mil_active);
    DiagStatus getStatus() const { return status; }
    const char* getSuggestedCause();
    void setSymptom(Symptom s) { currentSymptom = s; }

private:
    DiagStatus status;
    Symptom currentSymptom;
    
    // Internal Analyzers
    void analyzeBattery(const VehicleProfile& profile, float battery_v);
    void analyzeTPS(const VehicleProfile& profile, float tps_v);
    void analyzeIdle(const VehicleProfile& profile, float rpm, float temp);
    void analyzeCharging(const VehicleProfile& profile, float battery_v, float rpm);
};

#endif // DIAG_ENGINE_H
