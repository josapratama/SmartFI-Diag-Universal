/**
 * @file diagnostic_rules.h
 * @brief Advanced Rule-Based Diagnostic Engine for FI Motorcycles.
 */

#ifndef DIAGNOSTIC_RULES_H
#define DIAGNOSTIC_RULES_H

#include <Arduino.h>
#include <vector>
#include "config_types.h"

// ==========================================
// 1. DATA STRUCTURES FOR DIAGNOSIS
// ==========================================

enum Severity {
    SEV_INFO,
    SEV_LOW,
    SEV_MEDIUM,
    SEV_HIGH,
    SEV_CRITICAL
};

struct DiagnosisCandidate {
    String cause;
    int confidence; // 0-100
    Severity severity;
    std::vector<String> evidence;
    String next_check;
};

struct DiagnosticInput {
    float battery_v;
    float crank_min_v;
    float charging_v;
    float tps_v;
    float tps_pct;
    float rpm;
    float rpm_variance;
    float engine_temp;
    bool mil_active;
    int mil_code;
    bool injector_pulse;
    bool ignition_pulse;
    bool starter_signal;
};

// ==========================================
// 2. DIAGNOSTIC ENGINE CLASS
// ==========================================

class DiagnosticEngine {
public:
    DiagnosticEngine();
    
    void update(const VehicleProfile& profile, const DiagnosticInput& input);
    std::vector<DiagnosisCandidate> getResults() const { return results; }
    
    // Symptom-specific triggers
    void runSymptomAnalysis(int symptom_id, const VehicleProfile& profile, const DiagnosticInput& input);

private:
    std::vector<DiagnosisCandidate> results;

    // Rule Modules
    void checkUniversalRules(const VehicleProfile& profile, const DiagnosticInput& input);
    void checkHondaRules(const VehicleProfile& profile, const DiagnosticInput& input);
    void checkYamahaRules(const VehicleProfile& profile, const DiagnosticInput& input);

    // Helpers
    void addCandidate(const String& cause, int conf, Severity sev, const std::vector<String>& evidence, const String& next);
};

#endif // DIAGNOSTIC_RULES_H
