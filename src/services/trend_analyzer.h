/**
 * @file trend_analyzer.h
 * @brief Trend Analyzer - records time-series data for web chart display.
 */

#ifndef TREND_ANALYZER_H
#define TREND_ANALYZER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

struct TrendPoint {
    uint32_t timestamp;
    float battery_v;
    float rpm;
    float tps_pct;
    float temperature;
};

class TrendAnalyzer {
public:
    TrendAnalyzer() {}

    void record(float batt, float rpm, float tps_pct, float temp) {
        TrendPoint point;
        point.timestamp = millis() / 1000; // Seconds since boot
        point.battery_v = batt;
        point.rpm = rpm;
        point.tps_pct = tps_pct;
        point.temperature = temp;

        // Ring buffer: keep last MAX_POINTS
        if (points.size() >= MAX_POINTS) {
            points.erase(points.begin());
        }
        points.push_back(point);
    }

    String toJson() const {
        // Calculate required size: ~40 bytes per point * max 120 points + overhead
        size_t docSize = min((size_t)5120, points.size() * 40 + 256);
        DynamicJsonDocument doc(docSize);
        
        if (doc.capacity() == 0) {
            // Allocation failed, return minimal response
            return "{\"timestamps\":[],\"battery\":[],\"rpm\":[],\"tps\":[],\"temp\":[]}";
        }
        
        JsonArray tsArr = doc.createNestedArray("timestamps");
        JsonArray battArr = doc.createNestedArray("battery");
        JsonArray rpmArr = doc.createNestedArray("rpm");
        JsonArray tpsArr = doc.createNestedArray("tps");
        JsonArray tempArr = doc.createNestedArray("temp");

        // Downsample to max 60 points for JSON
        int step = 1;
        if (points.size() > 60) step = points.size() / 60;

        for (size_t i = 0; i < points.size(); i += step) {
            tsArr.add(points[i].timestamp);
            battArr.add(serialized(String(points[i].battery_v, 2)));
            rpmArr.add(serialized(String(points[i].rpm, 0)));
            tpsArr.add(serialized(String(points[i].tps_pct, 1)));
            tempArr.add(serialized(String(points[i].temperature, 1)));
        }

        String output;
        serializeJson(doc, output);
        return output;
    }

    // Statistics
    float getAvgRPM() const {
        if (points.empty()) return 0;
        float sum = 0;
        int count = 0;
        for (const auto& p : points) {
            if (p.rpm > 0) { sum += p.rpm; count++; }
        }
        return count > 0 ? sum / count : 0;
    }

    float getMaxRPM() const {
        float maxVal = 0;
        for (const auto& p : points) {
            if (p.rpm > maxVal) maxVal = p.rpm;
        }
        return maxVal;
    }

    float getMinBattery() const {
        float minVal = 99.0f;
        for (const auto& p : points) {
            if (p.battery_v > 0 && p.battery_v < minVal) minVal = p.battery_v;
        }
        return minVal < 99.0f ? minVal : 0;
    }

    void clear() { points.clear(); }

private:
    std::vector<TrendPoint> points;
    static const size_t MAX_POINTS = 300; // 5 minutes at 1Hz
};

#endif // TREND_ANALYZER_H
