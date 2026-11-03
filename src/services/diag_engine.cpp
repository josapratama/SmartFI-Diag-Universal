#include "diag_engine.h"

DiagEngine::DiagEngine() {
    status = {false, false, false, false, false, false, false, false, 100.0f};
    currentSymptom = SYMPTOM_NONE;
}

void DiagEngine::update(const VehicleProfile& profile, float battery_v, float tps_v, float rpm, float temp, bool mil_active) {
    analyzeBattery(profile, battery_v);
    analyzeTPS(profile, tps_v);
    analyzeIdle(profile, rpm, temp);
    analyzeCharging(profile, battery_v, rpm);
    
    // Update status.mil_error_detected from external lamp reader
    status.mil_error_detected = mil_active;
}

void DiagEngine::analyzeBattery(const VehicleProfile& profile, float battery_v) {
    if (battery_v < profile.thresholds.batt_v_idle_min && battery_v > 5.0f) {
        status.low_battery = true;
    } else {
        status.low_battery = false;
    }
}

void DiagEngine::analyzeTPS(const VehicleProfile& profile, float tps_v) {
    // Detect out of range with safety margin
    if (tps_v < profile.thresholds.tps_v_idle - 0.25f || tps_v > profile.thresholds.tps_v_full + 0.35f) {
        status.tps_out_of_range = true;
    } else {
        status.tps_out_of_range = false;
    }
}

void DiagEngine::analyzeIdle(const VehicleProfile& profile, float rpm, float temp) {
    // Only analyze idle stability when engine is warm (temp is in °C from TemperatureService)
    // Consider engine "warm" above 60°C
    if (temp >= 60.0f) {
        if (rpm > 0 && (rpm < profile.thresholds.rpm_idle_min || rpm > profile.thresholds.rpm_idle_max)) {
            status.idle_unstable = true;
        } else {
            status.idle_unstable = false;
        }
    } else {
        // Engine cold - can't assess idle stability
        status.idle_unstable = false;
    }
}

void DiagEngine::analyzeCharging(const VehicleProfile& profile, float battery_v, float rpm) {
    // IMPROVEMENT #3: Realistic automotive motorcycle charging curve
    // At high RPM (>2000 RPM), regulator/rectifier should produce 12.8V - 15.0V.
    if (rpm > 2000.0f) {
        const float CHARGING_V_MIN = 12.8f;
        const float CHARGING_V_MAX = 15.2f; // Overvoltage threshold (faulty regulator)
        
        if (battery_v < CHARGING_V_MIN || battery_v > CHARGING_V_MAX) {
            status.charging_failure = true;
        } else {
            status.charging_failure = false;
        }
    } else {
        // Can't assess charging at low RPM - reset flag
        status.charging_failure = false;
    }
}
