/**
 * @file cranking_analyzer.h
 * @brief Cranking Analyzer - detailed starter event analysis.
 */

#ifndef CRANKING_ANALYZER_H
#define CRANKING_ANALYZER_H

#include <Arduino.h>
#include <vector>
#include "config_types.h"

struct CrankEvent {
    uint32_t timestamp;
    uint32_t duration_ms;
    float min_voltage;
    float max_voltage;
    float voltage_drop;
    float pre_crank_voltage;
    bool engine_started;
    String verdict;
};

class CrankingAnalyzer {
public:
    CrankingAnalyzer() : isCranking(false), crankStartTime(0), 
                          minV(99.0f), maxV(0.0f), preCrankV(0.0f) {}

    void update(const VehicleProfile& profile, bool starterActive, float battV, float rpm) {
        if (starterActive && !isCranking) {
            // Cranking just started
            isCranking = true;
            crankStartTime = millis();
            preCrankV = battV;
            minV = battV;
            maxV = battV;
        } 
        else if (starterActive && isCranking) {
            // During cranking - track min/max
            if (battV < minV) minV = battV;
            if (battV > maxV) maxV = battV;
        }
        else if (!starterActive && isCranking) {
            // Cranking just ended - record event
            isCranking = false;
            uint32_t duration = millis() - crankStartTime;

            // Only record if cranking lasted more than 100ms (filter noise)
            if (duration > 100) {
                CrankEvent event;
                event.timestamp = crankStartTime;
                event.duration_ms = duration;
                event.min_voltage = minV;
                event.max_voltage = maxV;
                event.voltage_drop = preCrankV - minV;
                event.pre_crank_voltage = preCrankV;
                event.engine_started = (rpm > 500.0f); // Engine caught if RPM > 500

                // Generate verdict
                event.verdict = generateVerdict(profile, event);

                // Keep last MAX_EVENTS
                if (events.size() >= MAX_EVENTS) {
                    events.erase(events.begin());
                }
                events.push_back(event);

                Serial.printf("[CrankAnalyzer] Event: %dms, drop=%.2fV, min=%.2fV, started=%d\n",
                    event.duration_ms, event.voltage_drop, event.min_voltage, event.engine_started);
            }

            // Reset
            minV = 99.0f;
            maxV = 0.0f;
        }
    }

    const std::vector<CrankEvent>& getEvents() const { return events; }

    CrankEvent getLastEvent() const {
        if (events.empty()) {
            return {0, 0, 0, 0, 0, 0, false, "No data"};
        }
        return events.back();
    }

    void clear() { events.clear(); }

private:
    bool isCranking;
    uint32_t crankStartTime;
    float minV, maxV, preCrankV;
    std::vector<CrankEvent> events;
    static const size_t MAX_EVENTS = 10;

    String generateVerdict(const VehicleProfile& profile, const CrankEvent& ev) {
        // Battery health assessment
        if (ev.min_voltage < 8.0f) {
            return "KRITIS: Aki sangat lemah, perlu diganti segera";
        }
        if (ev.min_voltage < profile.startup.batt_crank_min) {
            return "PERINGATAN: Tegangan drop terlalu rendah (" + 
                   String(ev.min_voltage, 1) + "V) - aki mulai lemah";
        }
        
        // Duration assessment
        if (ev.duration_ms > (uint32_t)profile.startup.crank_duration_max_ms) {
            if (!ev.engine_started) {
                return "GAGAL: Cranking terlalu lama tanpa start - cek busi/fuel";
            }
            return "LAMBAT: Mesin hidup tapi cranking lama - monitor aki";
        }

        // Voltage drop assessment
        if (ev.voltage_drop > 4.0f) {
            return "DROP TINGGI: Voltage drop " + String(ev.voltage_drop, 1) + 
                   "V - cek kabel massa dan terminal aki";
        }
        if (ev.voltage_drop > 3.0f) {
            return "NORMAL-BATAS: Drop " + String(ev.voltage_drop, 1) + "V - masih OK";
        }

        if (ev.engine_started) {
            return "BAIK: Start normal (" + String(ev.duration_ms) + "ms, drop " + 
                   String(ev.voltage_drop, 1) + "V)";
        }

        return "GAGAL START: Cek sistem pengapian dan bahan bakar";
    }
};

#endif // CRANKING_ANALYZER_H
