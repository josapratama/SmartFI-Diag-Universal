/**
 * @file diagnostic_wizard.h
 * @brief Guided Diagnostic Wizard - symptom-based step-by-step diagnosis.
 */

#ifndef DIAGNOSTIC_WIZARD_H
#define DIAGNOSTIC_WIZARD_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "config_types.h"

// ==========================================
// SYMPTOM DEFINITIONS
// ==========================================
enum WizardSymptom {
    WIZ_ROUGH_START = 1,
    WIZ_HARD_START_COLD = 2,
    WIZ_LOSS_OF_IDLE = 3,
    WIZ_STALL_OFF_THROTTLE = 4,
    WIZ_BREBET_ACCEL = 5,
    WIZ_HIGH_FUEL_CONSUMPTION = 6,
    WIZ_OVERHEAT = 7,
    WIZ_NO_START = 8,
    WIZ_MIL_ON = 9
};

struct WizardStep {
    String instruction;    // What to check/do
    String expected;       // Expected result
    String if_fail;        // What it means if check fails
};

struct WizardResult {
    String diagnosis;
    int confidence;        // 0-100
    String suggested_fix;
    String severity;       // "info", "warning", "critical"
    std::vector<String> evidence;
};

struct SensorSnapshot {
    float battery_v;
    float tps_v;
    float tps_pct;
    float rpm;
    float rpm_variance;
    float temperature;
    bool mil_active;
    int mil_code;
    uint32_t injector_pw_us;
};

class DiagnosticWizard {
public:
    DiagnosticWizard() : activeSymptom(0) {}

    // Get available symptoms list
    String getSymptomListJson() const {
        DynamicJsonDocument doc(1024);
        JsonArray arr = doc.to<JsonArray>();
        
        addSymptom(arr, WIZ_ROUGH_START, "Starter Kasar / Rough Start");
        addSymptom(arr, WIZ_HARD_START_COLD, "Susah Starter Pagi / Hard Cold Start");
        addSymptom(arr, WIZ_LOSS_OF_IDLE, "Langsam Hilang / Loss of Idle");
        addSymptom(arr, WIZ_STALL_OFF_THROTTLE, "Mati Saat Lepas Gas / Stall");
        addSymptom(arr, WIZ_BREBET_ACCEL, "Brebet Saat Akselerasi");
        addSymptom(arr, WIZ_HIGH_FUEL_CONSUMPTION, "Boros BBM / High Consumption");
        addSymptom(arr, WIZ_OVERHEAT, "Mesin Panas Berlebih / Overheat");
        addSymptom(arr, WIZ_NO_START, "Tidak Bisa Starter / No Start");
        addSymptom(arr, WIZ_MIL_ON, "Lampu FI Menyala / MIL On");

        String output;
        serializeJson(doc, output);
        return output;
    }

    // Run analysis for a given symptom with current sensor data
    WizardResult analyze(int symptomId, const VehicleProfile& profile, const SensorSnapshot& snap) {
        activeSymptom = symptomId;
        WizardResult result;
        result.confidence = 0;
        result.severity = "info";

        switch (symptomId) {
            case WIZ_ROUGH_START:
                result = analyzeRoughStart(profile, snap);
                break;
            case WIZ_HARD_START_COLD:
                result = analyzeHardColdStart(profile, snap);
                break;
            case WIZ_LOSS_OF_IDLE:
                result = analyzeLossOfIdle(profile, snap);
                break;
            case WIZ_STALL_OFF_THROTTLE:
                result = analyzeStall(profile, snap);
                break;
            case WIZ_BREBET_ACCEL:
                result = analyzeBrebet(profile, snap);
                break;
            case WIZ_HIGH_FUEL_CONSUMPTION:
                result = analyzeHighConsumption(profile, snap);
                break;
            case WIZ_OVERHEAT:
                result = analyzeOverheat(profile, snap);
                break;
            case WIZ_NO_START:
                result = analyzeNoStart(profile, snap);
                break;
            case WIZ_MIL_ON:
                result = analyzeMILOn(profile, snap);
                break;
            default:
                result.diagnosis = "Gejala tidak dikenali";
                break;
        }

        return result;
    }

    String resultToJson(const WizardResult& result) const {
        DynamicJsonDocument doc(1024);
        doc["diagnosis"] = result.diagnosis;
        doc["confidence"] = result.confidence;
        doc["fix"] = result.suggested_fix;
        doc["severity"] = result.severity;
        JsonArray ev = doc.createNestedArray("evidence");
        for (const auto& e : result.evidence) ev.add(e);

        String output;
        serializeJson(doc, output);
        return output;
    }

private:
    int activeSymptom;

    void addSymptom(JsonArray& arr, int id, const char* name) const {
        JsonObject obj = arr.createNestedObject();
        obj["id"] = id;
        obj["name"] = name;
    }

    // ==========================================
    // ANALYSIS FUNCTIONS
    // ==========================================

    WizardResult analyzeRoughStart(const VehicleProfile& profile, const SensorSnapshot& snap) {
        WizardResult r;
        if (snap.battery_v < profile.startup.batt_crank_min) {
            r.diagnosis = "Aki lemah - tegangan drop saat cranking";
            r.confidence = 85;
            r.suggested_fix = "Charge atau ganti aki. Cek terminal aki.";
            r.severity = "warning";
            r.evidence.push_back("Battery: " + String(snap.battery_v, 2) + "V < " + String(profile.startup.batt_crank_min, 1) + "V");
        } else if (snap.injector_pw_us == 0) {
            r.diagnosis = "Injektor tidak menyemprot saat cranking";
            r.confidence = 90;
            r.suggested_fix = "Cek sinyal injektor, relay FI, dan fuse.";
            r.severity = "critical";
            r.evidence.push_back("Injector PW = 0 us");
        } else {
            r.diagnosis = "Kemungkinan busi aus atau tekanan fuel pump rendah";
            r.confidence = 60;
            r.suggested_fix = "Ganti busi, cek tekanan fuel pump (3 bar).";
            r.severity = "info";
            r.evidence.push_back("Battery OK, Injector OK");
        }
        return r;
    }

    WizardResult analyzeHardColdStart(const VehicleProfile& profile, const SensorSnapshot& snap) {
        WizardResult r;
        if (snap.temperature < 30.0f && snap.injector_pw_us < 3000) {
            r.diagnosis = "Enrichment dingin kurang - ECU tidak menambah BBM";
            r.confidence = 75;
            r.suggested_fix = "Cek sensor ECT (suhu mesin). Jika rusak, ECU tidak tahu mesin dingin.";
            r.severity = "warning";
            r.evidence.push_back("Temp: " + String(snap.temperature, 1) + "C, INJ PW: " + String(snap.injector_pw_us) + "us");
        } else if (snap.battery_v < 12.0f) {
            r.diagnosis = "Aki lemah di pagi hari (self-discharge)";
            r.confidence = 80;
            r.suggested_fix = "Cek aki (mungkin sudah tua). Cek parasitic drain.";
            r.severity = "warning";
            r.evidence.push_back("Battery: " + String(snap.battery_v, 2) + "V");
        } else {
            r.diagnosis = "Kemungkinan fuel pump priming kurang atau busi basah";
            r.confidence = 50;
            r.suggested_fix = "Putar kunci ON 3 detik sebelum starter (priming). Cek busi.";
            r.severity = "info";
        }
        return r;
    }

    WizardResult analyzeLossOfIdle(const VehicleProfile& profile, const SensorSnapshot& snap) {
        WizardResult r;
        if (snap.tps_v > profile.thresholds.tps_v_idle + 0.3f && snap.rpm == 0) {
            r.diagnosis = "TPS tidak kembali ke posisi idle";
            r.confidence = 90;
            r.suggested_fix = "Kalibrasi TPS atau cek kabel gas nyangkut.";
            r.severity = "warning";
            r.evidence.push_back("TPS: " + String(snap.tps_v, 2) + "V (idle seharusnya " + String(profile.thresholds.tps_v_idle, 2) + "V)");
        } else if (snap.rpm_variance > 150.0f) {
            r.diagnosis = "Idle tidak stabil - kemungkinan IACV/ISC kotor";
            r.confidence = 80;
            r.suggested_fix = "Bersihkan Throttle Body dan IACV/ISC. Cek kebocoran intake.";
            r.severity = "warning";
            r.evidence.push_back("RPM variance: " + String(snap.rpm_variance, 0));
        } else {
            r.diagnosis = "Kemungkinan kompresi rendah atau timing off";
            r.confidence = 50;
            r.suggested_fix = "Cek kompresi silinder. Cek timing chain/belt.";
            r.severity = "info";
        }
        return r;
    }

    WizardResult analyzeStall(const VehicleProfile& profile, const SensorSnapshot& snap) {
        WizardResult r;
        if (snap.tps_pct < 5.0f && snap.rpm_variance > 200.0f) {
            r.diagnosis = "RPM drop drastis saat lepas gas - IACV gagal kompensasi";
            r.confidence = 85;
            r.suggested_fix = "Bersihkan IACV/ISC. Cek vacuum hose.";
            r.severity = "warning";
            r.evidence.push_back("TPS: " + String(snap.tps_pct, 1) + "%, RPM var: " + String(snap.rpm_variance, 0));
        } else {
            r.diagnosis = "Kemungkinan fuel cut-off atau sensor MAP error";
            r.confidence = 60;
            r.suggested_fix = "Cek sensor MAP dan vacuum line.";
            r.severity = "info";
        }
        return r;
    }

    WizardResult analyzeBrebet(const VehicleProfile& profile, const SensorSnapshot& snap) {
        WizardResult r;
        if (snap.tps_pct > 30.0f && snap.injector_pw_us < 2000) {
            r.diagnosis = "Campuran terlalu kurus saat akselerasi (lean)";
            r.confidence = 80;
            r.suggested_fix = "Cek fuel pump pressure, filter BBM, dan injektor (mungkin tersumbat).";
            r.severity = "warning";
            r.evidence.push_back("TPS: " + String(snap.tps_pct, 0) + "% tapi INJ PW hanya " + String(snap.injector_pw_us) + "us");
        } else if (snap.tps_pct > 30.0f && snap.injector_pw_us > 8000) {
            r.diagnosis = "Campuran terlalu kaya (rich) - kemungkinan sensor O2 rusak";
            r.confidence = 75;
            r.suggested_fix = "Cek sensor O2 dan sensor MAP. Cek filter udara.";
            r.severity = "warning";
            r.evidence.push_back("INJ PW tinggi: " + String(snap.injector_pw_us) + "us");
        } else {
            r.diagnosis = "Kemungkinan ignition miss atau busi aus";
            r.confidence = 60;
            r.suggested_fix = "Ganti busi. Cek coil dan kabel busi.";
            r.severity = "info";
        }
        return r;
    }

    WizardResult analyzeHighConsumption(const VehicleProfile& profile, const SensorSnapshot& snap) {
        WizardResult r;
        if (snap.injector_pw_us > 5000 && snap.rpm < 2000) {
            r.diagnosis = "Injector pulse width terlalu lebar di RPM rendah";
            r.confidence = 80;
            r.suggested_fix = "Cek sensor O2, ECT, dan MAP. Kemungkinan ECU over-fueling.";
            r.severity = "warning";
            r.evidence.push_back("INJ PW: " + String(snap.injector_pw_us) + "us @ " + String(snap.rpm, 0) + " RPM");
        } else if (snap.tps_v > profile.thresholds.tps_v_idle + 0.2f && snap.rpm < 1500) {
            r.diagnosis = "TPS idle terlalu tinggi - throttle tidak menutup sempurna";
            r.confidence = 70;
            r.suggested_fix = "Setel kabel gas. Bersihkan throttle body.";
            r.severity = "info";
            r.evidence.push_back("TPS idle: " + String(snap.tps_v, 2) + "V");
        } else {
            r.diagnosis = "Cek tekanan ban, gaya berkendara, atau filter udara kotor";
            r.confidence = 40;
            r.suggested_fix = "Ganti filter udara. Cek tekanan ban. Hindari akselerasi mendadak.";
            r.severity = "info";
        }
        return r;
    }

    WizardResult analyzeOverheat(const VehicleProfile& profile, const SensorSnapshot& snap) {
        WizardResult r;
        if (snap.temperature > 105.0f) {
            r.diagnosis = "Mesin overheat - suhu kritis";
            r.confidence = 95;
            r.suggested_fix = "MATIKAN MESIN. Cek coolant/oli, radiator, thermostat, water pump.";
            r.severity = "critical";
            r.evidence.push_back("Temp: " + String(snap.temperature, 1) + "C");
        } else if (snap.temperature > 95.0f) {
            r.diagnosis = "Suhu mendekati batas - potensi overheat";
            r.confidence = 70;
            r.suggested_fix = "Kurangi beban. Cek level coolant dan kipas radiator.";
            r.severity = "warning";
            r.evidence.push_back("Temp: " + String(snap.temperature, 1) + "C");
        } else {
            r.diagnosis = "Suhu masih normal - monitor terus";
            r.confidence = 30;
            r.suggested_fix = "Pastikan sirkulasi udara baik. Jangan macet terlalu lama.";
            r.severity = "info";
        }
        return r;
    }

    WizardResult analyzeNoStart(const VehicleProfile& profile, const SensorSnapshot& snap) {
        WizardResult r;
        if (snap.battery_v < 9.0f) {
            r.diagnosis = "Aki mati total - tidak cukup daya untuk starter";
            r.confidence = 95;
            r.suggested_fix = "Jump start atau ganti aki.";
            r.severity = "critical";
            r.evidence.push_back("Battery: " + String(snap.battery_v, 2) + "V");
        } else if (snap.battery_v < profile.startup.batt_crank_min) {
            r.diagnosis = "Aki lemah - starter berputar lambat";
            r.confidence = 85;
            r.suggested_fix = "Charge aki. Cek terminal dan kabel massa.";
            r.severity = "warning";
            r.evidence.push_back("Battery: " + String(snap.battery_v, 2) + "V");
        } else if (snap.mil_active) {
            r.diagnosis = "ECU mendeteksi error - cek kode MIL";
            r.confidence = 80;
            r.suggested_fix = "Baca kode error FI (MIL code: " + String(snap.mil_code) + "). Perbaiki sesuai kode.";
            r.severity = "warning";
            r.evidence.push_back("MIL active, code: " + String(snap.mil_code));
        } else {
            r.diagnosis = "Cek kill switch, side stand switch, atau relay starter";
            r.confidence = 60;
            r.suggested_fix = "Pastikan kill switch OFF, standar samping terangkat, kopling ditarik (jika ada).";
            r.severity = "info";
        }
        return r;
    }

    WizardResult analyzeMILOn(const VehicleProfile& profile, const SensorSnapshot& snap) {
        WizardResult r;
        if (snap.mil_code > 0) {
            r.diagnosis = "Kode error FI terdeteksi: " + String(snap.mil_code);
            r.confidence = 90;
            r.suggested_fix = "Lihat tabel DTC untuk kode " + String(snap.mil_code) + ". Perbaiki komponen terkait.";
            r.severity = "warning";
            r.evidence.push_back("MIL code: " + String(snap.mil_code));
        } else {
            r.diagnosis = "MIL menyala tapi kode belum terbaca";
            r.confidence = 50;
            r.suggested_fix = "Tunggu hingga ECU selesai mengirim kode (hitung kedipan lampu FI).";
            r.severity = "info";
        }
        return r;
    }
};

#endif // DIAGNOSTIC_WIZARD_H
