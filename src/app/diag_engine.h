/**
 * @file diag_engine.h
 * @brief Rule-based Diagnostic Engine for Smart FI Diagnostic Tool.
 */

#ifndef APP_DIAG_ENGINE_H
#define APP_DIAG_ENGINE_H

#include <Arduino.h>
#include <vector>
#include "config_types.h"
#include "services/battery_service.h"

class AppDiagEngine {
public:
    AppDiagEngine() : currentSymptom(0) {}

    void init(const VehicleProfile& profile) {
        this->profile = profile;
    }

    void update(float batteryV, float tpsV, float rpm, float temp) {
        // Apply Rules
        status.low_battery = (batteryV < profile.startup.batt_crank_min);
        status.tps_error = (tpsV < profile.tps_idle_v - 0.2f || tpsV > profile.tps_full_v + 0.3f);
        status.idle_unstable = (rpm > 0 && (rpm < profile.idle_rpm_min || rpm > profile.idle_rpm_max));
        
        // Symptom-Based Logic
        analyzeSymptoms(batteryV, tpsV, rpm, temp);
    }

    struct DiagStatus {
        bool low_battery;
        bool tps_error;
        bool idle_unstable;
        String suggested_action;
    } status;

    void setSymptom(int s) { currentSymptom = s; }

private:
    VehicleProfile profile;
    int currentSymptom;

    void analyzeSymptoms(float batteryV, float tpsV, float rpm, float temp) {
        if (currentSymptom == 1) { // Rough Start
            if (status.low_battery) status.suggested_action = "Aki lemah (Battery weak)";
            else status.suggested_action = "Cek busi atau tekanan fuel pump";
        } else if (currentSymptom == 2) { // Loss of Idle
            if (status.tps_error) status.suggested_action = "Kalibrasi TPS (TPS Error)";
            else status.suggested_action = "Bersihkan Throttle Body/IACV";
        }
    }
};

#endif // APP_DIAG_ENGINE_H
