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
    // Detect out of range
    if (tps_v < profile.thresholds.tps_v_idle - 0.2f || tps_v > profile.thresholds.tps_v_full + 0.3f) {
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
    if (rpm > 2000.0f) {
        if (battery_v < profile.thresholds.batt_v_idle_min || battery_v > profile.thresholds.batt_v_idle_max) {
            status.charging_failure = true;
        } else {
            status.charging_failure = false;
        }
    } else {
        // Can't assess charging at low RPM - reset flag
        status.charging_failure = false;
    }
}

const char* DiagEngine::getSuggestedCause() {
    // Rule Engine for Symptom-Based Guided Diagnosis
    if (currentSymptom == SYMPTOM_ROUGH_START) {
        if (status.low_battery) return "Aki lemah (Battery weak)";
        if (status.ignition_missing) return "Pengapian hilang (Ignition failure)";
        return "Cek busi atau tekanan fuel pump (Check plug or fuel pressure)";
    }
    
    if (currentSymptom == SYMPTOM_LOSS_OF_IDLE || currentSymptom == SYMPTOM_STALL_ON_OFF_THROTTLE) {
        if (status.tps_out_of_range) return "TPS error/kalibrasi (TPS calibration error)";
        if (status.idle_unstable) return "Cek IACV/TB kotor atau setelan idle (Check IACV/TB sync)";
        return "Cek kompresi/klep atau kebocoran intake (Check compression/intake leak)";
    }
    
    if (status.charging_failure) return "Masalah pengisian/kiprok (Charging/Regulator fault)";
    if (status.mil_error_detected) return "Cek kode MIL/DTC (Check MIL code)";
    
    return "Status Normal - Pantau live data (Normal - Monitor live data)";
}
