#include "diagnostic_rules.h"

DiagnosticEngine::DiagnosticEngine() {
    results.clear();
}

void DiagnosticEngine::update(const VehicleProfile& profile, const DiagnosticInput& input) {
    results.clear();
    checkUniversalRules(profile, input);
    
    if (profile.name.indexOf("Honda") != -1) {
        checkHondaRules(profile, input);
    } else if (profile.name.indexOf("Yamaha") != -1) {
        checkYamahaRules(profile, input);
    }
}

void DiagnosticEngine::checkUniversalRules(const VehicleProfile& profile, const DiagnosticInput& input) {
    // 1. Battery & Charging
    if (input.battery_v < 12.0f && input.rpm == 0) {
        addCandidate("Aki lemah (Battery weak)", 80, SEV_MEDIUM, 
            {"Tegangan aki < 12V saat mesin mati"}, 
            "Cek charging atau ganti aki");
    }
    
    if (input.rpm > 2000.0f) {
        if (input.battery_v < 13.0f || input.battery_v > 15.0f) {
            addCandidate("Sistem pengisian bermasalah (Charging fault)", 90, SEV_HIGH,
                {"Tegangan saat mesin hidup diluar range (13.5V - 14.8V)"},
                "Cek kiprok/regulator dan spul");
        }
    }

    // 2. Overheat
    if (input.engine_temp > 100.0f) {
        addCandidate("Mesin overheat", 95, SEV_CRITICAL,
            {"Suhu mesin > 100 C"},
            "Cek air radiator atau sensor suhu");
    }

    // 3. TPS Sanity
    if (input.tps_v < 0.2f || input.tps_v > 4.8f) {
        addCandidate("Sensor TPS rusak/lepas", 100, SEV_HIGH,
            {"Tegangan TPS diluar range normal (0.4V - 4.5V)"},
            "Cek soket TPS dan kabel signal");
    }
}

void DiagnosticEngine::checkHondaRules(const VehicleProfile& profile, const DiagnosticInput& input) {
    // 1. ACG Starter Rough Start
    if (input.starter_signal && input.crank_min_v < 10.5f) {
        addCandidate("Starter kasar (ACG Fault)", 75, SEV_MEDIUM,
            {"Tegangan drop < 10.5V saat cranking", "Sistem Honda ACG sensitif terhadap tegangan"},
            "Cek koneksi relay ACG dan kabel massa");
    }

    // 2. Honda Idle Loss (PGM-FI)
    if (input.engine_temp > 70.0f && input.rpm > 0 && input.rpm < 1400.0f) {
        addCandidate("Langsam rendah/hilang (Honda PGM-FI)", 85, SEV_LOW,
            {"RPM idle < 1400 saat mesin panas", "TPS voltage stable"},
            "Bersihkan Throttle Body dan IACV");
    }
}

void DiagnosticEngine::checkYamahaRules(const VehicleProfile& profile, const DiagnosticInput& input) {
    // 1. Yamaha ISC Problem (BlueCore)
    if (input.rpm_variance > 150.0f && input.rpm > 0) {
        addCandidate("Langsam tidak stabil (Yamaha ISC)", 80, SEV_MEDIUM,
            {"Variansi RPM tinggi (> 150)", "Sistem BlueCore mengandalkan ISC"},
            "Cek/bersihkan ISC unit");
    }

    // 2. Yamaha TPS Range (Typically 0.6V at idle)
    if (input.tps_v > 0.7f && input.rpm == 0 && input.tps_pct == 0) {
        addCandidate("TPS Out of Spec (Yamaha)", 70, SEV_LOW,
            {"Tegangan TPS idle > 0.7V", "Standar Yamaha 0.6V - 0.7V"},
            "Kalibrasi TPS di Throttle Body");
    }
}

void DiagnosticEngine::runSymptomAnalysis(int symptom_id, const VehicleProfile& profile, const DiagnosticInput& input) {
    // Specific triggers for user-selected symptoms
    // 1: Starter Kasar, 2: Susah Starter Pagi, 3: Langsam Hilang, 4: Mati saat lepas gas
    results.clear();
    
    if (symptom_id == 4) { // Mati saat lepas gas
        if (input.rpm_variance > 100 && input.tps_v < 0.5f) {
            addCandidate("TPS Loss of Sync", 90, SEV_HIGH,
                {"RPM drop mendadak saat gas dilepas", "TPS voltage rendah"},
                "Kalibrasi TPS atau cek kebocoran intake");
        }
    }
}

void DiagnosticEngine::addCandidate(const String& cause, int conf, Severity sev, const std::vector<String>& evidence, const String& next) {
    DiagnosisCandidate c;
    c.cause = cause;
    c.confidence = conf;
    c.severity = sev;
    c.evidence = evidence;
    c.next_check = next;
    results.push_back(c);
}
