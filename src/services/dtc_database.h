/**
 * @file dtc_database.h
 * @brief DTC Database - loads DTC descriptions and fixes from SD card JSON.
 * Supports multi-language and brand-specific codes.
 */

#ifndef DTC_DATABASE_H
#define DTC_DATABASE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <map>

struct DTCEntry {
    String code;
    String description_id;  // Indonesian
    String description_en;  // English
    String suggested_fix;
    String severity;        // "low", "medium", "high", "critical"
    String component;       // Related component
};

class DTCDatabase {
public:
    static DTCDatabase& getInstance() {
        static DTCDatabase instance;
        return instance;
    }

    bool load(const String& brand) {
        entries.clear();
        currentBrand = brand;

        String path = "/configs/dtc/" + brand + "_dtc.json";
        File file = SD.open(path);
        
        if (!file) {
            Serial.println("[DTCDatabase] File not found: " + path + ", using built-in");
            loadBuiltIn(brand);
            return false;
        }

        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.println("[DTCDatabase] JSON parse error: " + String(error.c_str()));
            loadBuiltIn(brand);
            return false;
        }

        JsonArray arr = doc["codes"].as<JsonArray>();
        for (JsonObject obj : arr) {
            DTCEntry entry;
            entry.code = obj["code"].as<String>();
            entry.description_id = obj["desc_id"] | obj["desc"].as<const char*>();
            entry.description_en = obj["desc_en"] | "";
            entry.suggested_fix = obj["fix"] | "";
            entry.severity = obj["severity"] | "medium";
            entry.component = obj["component"] | "";
            entries[entry.code] = entry;
        }

        Serial.printf("[DTCDatabase] Loaded %d codes for %s\n", entries.size(), brand.c_str());
        return true;
    }

    String getDescription(const String& code, bool indonesian = true) const {
        auto it = entries.find(code);
        if (it != entries.end()) {
            return indonesian ? it->second.description_id : it->second.description_en;
        }
        return "Unknown DTC: " + code;
    }

    String getSuggestedFix(const String& code) const {
        auto it = entries.find(code);
        if (it != entries.end()) {
            return it->second.suggested_fix;
        }
        return "";
    }

    String getSeverity(const String& code) const {
        auto it = entries.find(code);
        if (it != entries.end()) {
            return it->second.severity;
        }
        return "medium";
    }

    String getComponent(const String& code) const {
        auto it = entries.find(code);
        if (it != entries.end()) {
            return it->second.component;
        }
        return "";
    }

    bool hasCode(const String& code) const {
        return entries.find(code) != entries.end();
    }

    String toJson() const {
        DynamicJsonDocument doc(4096);
        JsonArray arr = doc.to<JsonArray>();
        for (const auto& [code, entry] : entries) {
            JsonObject obj = arr.createNestedObject();
            obj["code"] = entry.code;
            obj["desc"] = entry.description_id;
            obj["fix"] = entry.suggested_fix;
            obj["severity"] = entry.severity;
            obj["component"] = entry.component;
        }
        String output;
        serializeJson(doc, output);
        return output;
    }

private:
    DTCDatabase() {}
    std::map<String, DTCEntry> entries;
    String currentBrand;

    void loadBuiltIn(const String& brand) {
        if (brand == "honda") {
            addEntry("1", "ECM/PCM Internal", "Ganti ECU", "critical", "ECU");
            addEntry("7", "Engine Speed Sensor", "Cek CKP sensor dan kabel", "high", "CKP");
            addEntry("8", "TDC Sensor", "Cek TDC sensor", "high", "TDC");
            addEntry("12", "Injector Fault", "Cek injektor dan kabel", "high", "Injector");
            addEntry("21", "MAP Sensor", "Cek MAP sensor dan vacuum hose", "medium", "MAP");
            addEntry("24", "O2 Sensor", "Cek sensor O2", "medium", "O2");
            addEntry("29", "IACV", "Bersihkan/ganti IACV", "medium", "IACV");
            addEntry("33", "ECM EEPROM", "Reset ECU atau ganti", "high", "ECU");
            addEntry("54", "Bank Angle Sensor", "Cek sensor sudut kemiringan", "low", "BAS");
        } else if (brand == "yamaha") {
            addEntry("12", "Crankshaft Position Sensor", "Cek CKP sensor dan gap", "high", "CKP");
            addEntry("14", "Intake Air Pressure Sensor", "Cek sensor MAP dan hose", "medium", "MAP");
            addEntry("19", "ECT Sensor", "Cek sensor suhu mesin dan kabel", "medium", "ECT");
            addEntry("21", "Intake Air Temp Sensor", "Cek sensor IAT", "low", "IAT");
            addEntry("22", "Intake Air Pressure Low", "Cek vacuum leak", "medium", "MAP");
            addEntry("23", "Intake Air Temp High", "Cek sensor IAT dan konektor", "medium", "IAT");
            addEntry("33", "Throttle Position Sensor", "Kalibrasi/ganti TPS", "high", "TPS");
            addEntry("39", "O2 Sensor Heated", "Ganti sensor O2", "medium", "O2");
            addEntry("41", "Lean Malfunction", "Cek fuel pump, injektor, kebocoran intake", "high", "Fuel");
            addEntry("42", "Rich Malfunction", "Cek filter udara, sensor O2, MAP", "medium", "Fuel");
            addEntry("44", "PAIR Solenoid", "Cek solenoid PAIR valve", "low", "Emission");
            addEntry("46", "Vehicle Speed Sensor", "Cek sensor speed dan gear driven", "low", "VSS");
            addEntry("49", "FI System ECU", "Reset ECU, jika tetap ganti ECU", "critical", "ECU");
            addEntry("51", "TPS Low", "Cek kabel TPS, kemungkinan short", "high", "TPS");
            addEntry("52", "TPS High", "Cek kabel TPS, kemungkinan open", "high", "TPS");
            addEntry("54", "Ignition Coil", "Cek coil dan kabel busi", "high", "Ignition");
        }
    }

    void addEntry(const String& code, const String& desc, const String& fix, 
                  const String& sev, const String& comp) {
        DTCEntry entry;
        entry.code = code;
        entry.description_id = desc;
        entry.description_en = desc;
        entry.suggested_fix = fix;
        entry.severity = sev;
        entry.component = comp;
        entries[code] = entry;
    }
};

#endif // DTC_DATABASE_H
