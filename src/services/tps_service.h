/**
 * @file tps_service.h
 * @brief TPS Monitoring Service with Interactive Zero/WOT Calibration and NVS Storage.
 */

#ifndef TPS_SERVICE_H
#define TPS_SERVICE_H

#include <memory>
#include <Preferences.h>
#include "core/channel_manager.h"
#include "config_types.h"

class TPSService {
public:
    TPSService() 
        : tpsChan(nullptr), currentVoltage(0.0f), currentPercent(0.0f),
          customCalibrated(false), calIdleVoltage(0.5f), calWotVoltage(4.2f),
          tpsNoiseDetected(false), lastRawVoltage(0.0f) {}

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
        // Average 10 readings for stability
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
        // Average 10 readings for stability
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
        Serial.println("[TPSService] Calibration reset to profile defaults.");
    }

    void update(const VehicleProfile& profile) {
        if (!tpsChan || !profile.caps.has_tps) return;
        currentVoltage = tpsChan->read();

        // Check for TPS sensor noise / jump (erratic carbon track wear)
        float diff = abs(currentVoltage - lastRawVoltage);
        if (diff > 1.2f) { // Sudden 1.2V spike without transition
            tpsNoiseDetected = true;
        } else {
            tpsNoiseDetected = false;
        }
        lastRawVoltage = currentVoltage;
        
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

    float getVoltage() const { return currentVoltage; }
    float getPercent() const { return currentPercent; }
    bool isCustomCalibrated() const { return customCalibrated; }
    float getIdleCalibration() const { return calIdleVoltage; }
    float getWOTCalibration() const { return calWotVoltage; }
    bool hasNoiseOrFault() const { return tpsNoiseDetected; }

private:
    std::shared_ptr<AnalogDriver> tpsChan;
    float currentVoltage;
    float currentPercent;
    bool customCalibrated;
    float calIdleVoltage;
    float calWotVoltage;
    bool tpsNoiseDetected;
    float lastRawVoltage;
};

#endif // TPS_SERVICE_H
