/**
 * @file tps_service.h
 * @brief TPS Monitoring Service with Oscilloscope Buffer, Noise Glitch Detection, and Interactive Zero/WOT Calibration.
 */

#ifndef TPS_SERVICE_H
#define TPS_SERVICE_H

#include <memory>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "core/channel_manager.h"
#include "config_types.h"

#define TPS_WAVEFORM_SAMPLES 100

class TPSService {
public:
    TPSService() 
        : tpsChan(nullptr), currentVoltage(0.0f), currentPercent(0.0f),
          customCalibrated(false), calIdleVoltage(0.5f), calWotVoltage(4.2f),
          tpsNoiseDetected(false), lastRawVoltage(0.0f), glitchCount(0),
          bufHead(0), sampleCount(0), lastSampleTime(0) {
        for (int i = 0; i < TPS_WAVEFORM_SAMPLES; i++) {
            voltageBuffer[i] = 0.0f;
        }
    }

    void init() {
        tpsChan = ChannelManager::getInstance().getAnalog("CH_TPS");
        loadCalibration();
    }

    void loadCalibration() {
        Preferences prefs;
        if (prefs.begin("tps_cal", true)) {
            customCalibrated = prefs.getBool("calibrated", false);
            if (customCalibrated) {
                calIdleVoltage = prefs.getFloat("idle_v", 0.5f);
                calWotVoltage = prefs.getFloat("wot_v", 4.2f);
                Serial.printf("[TPSService] Loaded calibration: Zero=%.3fV, WOT=%.3fV\n", calIdleVoltage, calWotVoltage);
            }
            prefs.end();
        }
    }

    void saveCalibration() {
        Preferences prefs;
        if (prefs.begin("tps_cal", false)) {
            prefs.putBool("calibrated", true);
            prefs.putFloat("idle_v", calIdleVoltage);
            prefs.putFloat("wot_v", calWotVoltage);
            prefs.end();
            customCalibrated = true;
            Serial.printf("[TPSService] Calibration saved: Zero=%.3fV, WOT=%.3fV\n", calIdleVoltage, calWotVoltage);
        }
    }

    bool calibrateZero() {
        if (!tpsChan) return false;
        float sum = 0.0f;
        for (int i = 0; i < 10; i++) {
            sum += tpsChan->read();
            delay(10);
        }
        float measured = sum / 10.0f;
        if (measured >= 0.1f && measured <= 1.5f) {
            calIdleVoltage = measured;
            saveCalibration();
            return true;
        }
        return false;
    }

    bool calibrateWOT() {
        if (!tpsChan) return false;
        float sum = 0.0f;
        for (int i = 0; i < 10; i++) {
            sum += tpsChan->read();
            delay(10);
        }
        float measured = sum / 10.0f;
        if (measured >= 3.0f && measured <= 4.9f && measured > calIdleVoltage) {
            calWotVoltage = measured;
            saveCalibration();
            return true;
        }
        return false;
    }

    void resetCalibration() {
        Preferences prefs;
        if (prefs.begin("tps_cal", false)) {
            prefs.clear();
            prefs.end();
        }
        customCalibrated = false;
        calIdleVoltage = 0.5f;
        calWotVoltage = 4.2f;
        glitchCount = 0;
        Serial.println("[TPSService] Calibration reset to profile defaults.");
    }

    void update(const VehicleProfile& profile) {
        if (!tpsChan || !profile.caps.has_tps) return;
        currentVoltage = tpsChan->read();

        // Check for TPS sensor noise / jump (erratic carbon track wear)
        float diff = abs(currentVoltage - lastRawVoltage);
        if (diff > 1.0f && lastRawVoltage > 0.05f) { // Sudden 1.0V jump without transition
            tpsNoiseDetected = true;
            glitchCount++;
        } else {
            tpsNoiseDetected = false;
        }
        lastRawVoltage = currentVoltage;
        
        // Circular buffer recording (~50Hz sampling rate, every 20ms)
        uint32_t now = millis();
        if (now - lastSampleTime >= 20) {
            lastSampleTime = now;
            voltageBuffer[bufHead] = currentVoltage;
            bufHead = (bufHead + 1) % TPS_WAVEFORM_SAMPLES;
            if (sampleCount < TPS_WAVEFORM_SAMPLES) sampleCount++;
        }

        // Use custom calibration if available, otherwise vehicle profile defaults
        float idleV = customCalibrated ? calIdleVoltage : profile.thresholds.tps_v_idle;
        float wotV = customCalibrated ? calWotVoltage : profile.thresholds.tps_v_full;

        float range = wotV - idleV;
        if (range > 0.5f) {
            currentPercent = ((currentVoltage - idleV) / range) * 100.0f;
            if (currentPercent < 0.0f) currentPercent = 0.0f;
            if (currentPercent > 100.0f) currentPercent = 100.0f;
        }
    }

    String getWaveformJson() const {
        DynamicJsonDocument doc(2048);
        doc["current_v"] = serialized(String(currentVoltage, 2));
        doc["current_pct"] = serialized(String(currentPercent, 1));
        doc["idle_v"] = serialized(String(calIdleVoltage, 2));
        doc["wot_v"] = serialized(String(calWotVoltage, 2));
        doc["glitches"] = glitchCount;
        doc["noise_detected"] = tpsNoiseDetected;
        doc["calibrated"] = customCalibrated;

        JsonArray arr = doc.createNestedArray("samples");
        int count = sampleCount;
        int start = (bufHead - count + TPS_WAVEFORM_SAMPLES) % TPS_WAVEFORM_SAMPLES;
        for (int i = 0; i < count; i++) {
            int idx = (start + i) % TPS_WAVEFORM_SAMPLES;
            arr.add(serialized(String(voltageBuffer[idx], 2)));
        }

        String output;
        serializeJson(doc, output);
        return output;
    }

    float getVoltage() const { return currentVoltage; }
    float getPercent() const { return currentPercent; }
    bool isCustomCalibrated() const { return customCalibrated; }
    float getIdleCalibration() const { return calIdleVoltage; }
    float getWOTCalibration() const { return calWotVoltage; }
    bool hasNoiseOrFault() const { return tpsNoiseDetected; }
    uint32_t getGlitchCount() const { return glitchCount; }

private:
    std::shared_ptr<AnalogDriver> tpsChan;
    float currentVoltage;
    float currentPercent;
    bool customCalibrated;
    float calIdleVoltage;
    float calWotVoltage;
    bool tpsNoiseDetected;
    float lastRawVoltage;
    uint32_t glitchCount;

    // Real-time oscilloscope buffer
    float voltageBuffer[TPS_WAVEFORM_SAMPLES];
    int bufHead;
    int sampleCount;
    uint32_t lastSampleTime;
};

#endif // TPS_SERVICE_H
