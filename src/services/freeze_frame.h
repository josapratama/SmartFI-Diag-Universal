/**
 * @file freeze_frame.h
 * @brief Freeze Frame Capture Service - snapshot sensor data on events.
 */

#ifndef FREEZE_FRAME_H
#define FREEZE_FRAME_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

struct FreezeFrame {
    uint32_t timestamp;
    String trigger;         // What caused the capture (MIL_ON, DTC_xx, MANUAL, etc.)
    float battery_v;
    float tps_v;
    float tps_pct;
    float rpm;
    float temperature;
    float rpm_variance;
    bool mil_active;
    int mil_code;
    uint32_t injector_pw_us;
    String vehicle_name;
};

class FreezeFrameService {
public:
    static FreezeFrameService& getInstance() {
        static FreezeFrameService instance;
        return instance;
    }

    void capture(const String& trigger, float batt, float tps_v, float tps_pct,
                 float rpm, float temp, float rpm_var, bool mil, int milCode,
                 uint32_t inj_pw, const String& vehicle) {
        FreezeFrame frame;
        frame.timestamp = millis();
        frame.trigger = trigger;
        frame.battery_v = batt;
        frame.tps_v = tps_v;
        frame.tps_pct = tps_pct;
        frame.rpm = rpm;
        frame.temperature = temp;
        frame.rpm_variance = rpm_var;
        frame.mil_active = mil;
        frame.mil_code = milCode;
        frame.injector_pw_us = inj_pw;
        frame.vehicle_name = vehicle;

        // Keep max 20 frames (ring buffer behavior)
        if (frames.size() >= MAX_FRAMES) {
            frames.erase(frames.begin());
        }
        frames.push_back(frame);

        Serial.printf("[FreezeFrame] Captured: %s (total: %d)\n", 
            trigger.c_str(), frames.size());
    }

    const std::vector<FreezeFrame>& getFrames() const { return frames; }

    String toJson() const {
        // Each frame ~200 bytes in JSON, max 20 frames = ~4000 + overhead
        DynamicJsonDocument doc(5120);
        JsonArray arr = doc.to<JsonArray>();

        for (const auto& f : frames) {
            JsonObject obj = arr.createNestedObject();
            obj["ts"] = f.timestamp;
            obj["trigger"] = f.trigger;
            obj["batt"] = serialized(String(f.battery_v, 2));
            obj["tps_v"] = serialized(String(f.tps_v, 2));
            obj["tps_pct"] = serialized(String(f.tps_pct, 1));
            obj["rpm"] = serialized(String(f.rpm, 0));
            obj["temp"] = serialized(String(f.temperature, 1));
            obj["rpm_var"] = serialized(String(f.rpm_variance, 1));
            obj["mil"] = f.mil_active;
            obj["mil_code"] = f.mil_code;
            obj["inj_pw"] = f.injector_pw_us;
            obj["vehicle"] = f.vehicle_name;
            
            // Check if we're running out of memory
            if (doc.overflowed()) {
                break;
            }
        }

        String output;
        serializeJson(doc, output);
        return output;
    }

    void clear() { frames.clear(); }

private:
    FreezeFrameService() {}
    std::vector<FreezeFrame> frames;
    static const int MAX_FRAMES = 20;
};

#endif // FREEZE_FRAME_H
