/**
 * @file cranking_analyzer.h
 * @brief Cranking Analyzer - starter event capture with 10ms voltage drop oscilloscope waveform.
 */

#ifndef CRANKING_ANALYZER_H
#define CRANKING_ANALYZER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "config_types.h"

#define CRANK_WAVEFORM_MAX 150

struct CrankEvent {
    uint32_t timestamp;
    uint32_t duration_ms;
    float min_voltage;
    float max_voltage;
    float voltage_drop;
    float pre_crank_voltage;
    bool engine_started;
    String verdict;
    String grade;
};

class CrankingAnalyzer {
public:
    CrankingAnalyzer() : isCranking(false), crankStartTime(0), 
                          minV(99.0f), maxV(0.0f), preCrankV(0.0f),
                          waveformCount(0), lastSampleMs(0) {
        for (int i = 0; i < CRANK_WAVEFORM_MAX; i++) {
            waveformBuffer[i] = 0.0f;
        }
    }

    void update(const VehicleProfile& profile, bool starterActive, float battV, float rpm) {
        uint32_t now = millis();

        if (starterActive && !isCranking) {
            // Cranking just started
            isCranking = true;
            crankStartTime = now;
            preCrankV = battV;
            minV = battV;
            maxV = battV;
            waveformCount = 0;
            lastSampleMs = now;
            if (waveformCount < CRANK_WAVEFORM_MAX) {
                waveformBuffer[waveformCount++] = battV;
            }
        } 
        else if (starterActive && isCranking) {
            // During cranking - track min/max and record 10ms ADC voltage
            if (battV < minV) minV = battV;
            if (battV > maxV) maxV = battV;

            if (now - lastSampleMs >= 10 && waveformCount < CRANK_WAVEFORM_MAX) {
                lastSampleMs = now;
                waveformBuffer[waveformCount++] = battV;
            }
        }
        else if (!starterActive && isCranking) {
            // Cranking just ended - record event
            isCranking = false;
            uint32_t duration = now - crankStartTime;

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

                // Generate grade & verdict
                if (minV >= 10.5f) {
                    event.grade = "A (Prima)";
                    event.verdict = "Aki Sangat Sehat - Tegangan starter stabil di atas 10.5V";
                } else if (minV >= 9.5f) {
                    event.grade = "B (Normal)";
                    event.verdict = "Aki Baik - Kapasitas cranking optimal (9.5V - 10.5V)";
                } else if (minV >= 8.5f) {
                    event.grade = "C (Waspada)";
                    event.verdict = "Aki Mulai Lemah - Drop signifikan (8.5V - 9.5V), disarankan recharge";
                } else {
                    event.grade = "D (Kritis)";
                    event.verdict = "Aki Rusak/Soak - Tegangan anjlok di bawah 8.5V, ganti aki segera!";
                }

                // Keep last MAX_EVENTS
                if (events.size() >= MAX_EVENTS) {
                    events.erase(events.begin());
                }
                events.push_back(event);

                Serial.printf("[CrankAnalyzer] Event: %dms, drop=%.2fV, min=%.2fV, Grade=%s, started=%d\n",
                    event.duration_ms, event.voltage_drop, event.min_voltage, event.grade.c_str(), event.engine_started);
            }

            // Reset
            minV = 99.0f;
            maxV = 0.0f;
        }
    }

    String getWaveformJson() const {
        DynamicJsonDocument doc(2048);
        doc["is_cranking"] = isCranking;
        doc["count"] = waveformCount;

        CrankEvent last = getLastEvent();
        doc["last_min_v"] = serialized(String(last.min_voltage, 2));
        doc["last_pre_v"] = serialized(String(last.pre_crank_voltage, 2));
        doc["last_drop_v"] = serialized(String(last.voltage_drop, 2));
        doc["last_duration_ms"] = last.duration_ms;
        doc["grade"] = last.grade;
        doc["verdict"] = last.verdict;

        JsonArray arr = doc.createNestedArray("waveform");
        for (int i = 0; i < waveformCount; i++) {
            arr.add(serialized(String(waveformBuffer[i], 2)));
        }

        String output;
        serializeJson(doc, output);
        return output;
    }

    const std::vector<CrankEvent>& getEvents() const { return events; }

    CrankEvent getLastEvent() const {
        if (events.empty()) {
            return {0, 0, 12.6f, 12.6f, 0.0f, 12.6f, false, "Belum ada tes starter", "N/A"};
        }
        return events.back();
    }

    void clear() { 
        events.clear();
        waveformCount = 0;
    }

private:
    bool isCranking;
    uint32_t crankStartTime;
    float minV, maxV, preCrankV;
    std::vector<CrankEvent> events;
    static const size_t MAX_EVENTS = 10;

    float waveformBuffer[CRANK_WAVEFORM_MAX];
    int waveformCount;
    uint32_t lastSampleMs;
};

#endif // CRANKING_ANALYZER_H
