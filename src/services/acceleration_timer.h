/**
 * @file acceleration_timer.h
 * @brief Virtual Acceleration Timer & Dynamic Wheel Horsepower/Torque Dyno Estimator.
 * Computes 0-60 km/h acceleration, peak WHP, and wheel torque (Nm) in real-time.
 */

#ifndef ACCELERATION_TIMER_H
#define ACCELERATION_TIMER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

enum AccelState {
    ACCEL_IDLE,
    ACCEL_ARMED,
    ACCEL_RUNNING,
    ACCEL_FINISHED,
    ACCEL_FAILED
};

class AccelerationTimer {
public:
    static AccelerationTimer& getInstance() {
        static AccelerationTimer instance;
        return instance;
    }

    void init() {
        Preferences prefs;
        if (prefs.begin("accel_perf", true)) {
            bestTime0_60 = prefs.getFloat("best_0_60", 99.99f);
            peakHorsepower = prefs.getFloat("peak_hp", 0.0f);
            peakTorqueNm = prefs.getFloat("peak_tq", 0.0f);
            bikeWeightKg = prefs.getFloat("bike_kg", 125.0f);
            riderWeightKg = prefs.getFloat("rider_kg", 70.0f);
            prefs.end();
        }
    }

    void setWeights(float bikeKg, float riderKg) {
        if (bikeKg > 30.0f && bikeKg < 400.0f) bikeWeightKg = bikeKg;
        if (riderKg > 30.0f && riderKg < 200.0f) riderWeightKg = riderKg;

        Preferences prefs;
        if (prefs.begin("accel_perf", false)) {
            prefs.putFloat("bike_kg", bikeWeightKg);
            prefs.putFloat("rider_kg", riderWeightKg);
            prefs.end();
        }
    }

    void arm() {
        state = ACCEL_ARMED;
        startTimeMs = 0;
        elapsedTimeSec = 0.0f;
        maxSpeedReached = 0.0f;
        runPeakHP = 0.0f;
        runPeakTorque = 0.0f;
        Serial.println("[AccelDyno] Armed. Ready for launch!");
    }

    void reset() {
        state = ACCEL_IDLE;
        startTimeMs = 0;
        elapsedTimeSec = 0.0f;
        maxSpeedReached = 0.0f;
        currentHP = 0.0f;
        currentTorqueNm = 0.0f;
    }

    void update(float currentSpeedKmH, float tpsPercent = 0.0f, float currentRpm = 0.0f) {
        uint32_t now = millis();
        float dt = (lastUpdateTime > 0 && now > lastUpdateTime) ? (float)(now - lastUpdateTime) / 1000.0f : 0.05f;
        if (dt <= 0.001f || dt > 0.5f) dt = 0.05f;
        lastUpdateTime = now;

        if (currentSpeedKmH > maxSpeedReached) {
            maxSpeedReached = currentSpeedKmH;
        }

        // Real-Time Virtual Dyno Physics
        float v1 = lastSpeedKmH / 3.6f; // m/s
        float v2 = currentSpeedKmH / 3.6f; // m/s
        float accel_ms2 = (v2 - v1) / dt;
        if (accel_ms2 < 0.0f) accel_ms2 = 0.0f; // Only positive power delivery

        float totalMass = bikeWeightKg + riderWeightKg; // kg
        float f_inertia = totalMass * accel_ms2; // Newtons
        float f_aero = 0.5f * 1.2f * 0.55f * 0.45f * (v2 * v2); // Drag (Cd*A ~ 0.25 m2)
        float f_roll = totalMass * 9.81f * 0.015f; // Rolling resistance
        float f_total = f_inertia + f_aero + f_roll;

        float powerWatts = f_total * v2;
        if (powerWatts < 0.0f || currentSpeedKmH < 3.0f) powerWatts = 0.0f;

        float rawHP = powerWatts / 745.7f;
        // Low pass filter
        currentHP = (currentHP * 0.7f) + (rawHP * 0.3f);
        if (currentHP > 150.0f) currentHP = 150.0f; // Cap sensible FI motorcycle power

        // Torque Calculation: Torque (Nm) = (Power Watts * 60) / (2 * pi * RPM)
        if (currentRpm > 600.0f) {
            float rawTorque = (powerWatts * 60.0f) / (2.0f * 3.14159f * currentRpm);
            currentTorqueNm = (currentTorqueNm * 0.7f) + (rawTorque * 0.3f);
            if (currentTorqueNm > 150.0f) currentTorqueNm = 150.0f;
        } else {
            currentTorqueNm = 0.0f;
        }

        if (currentHP > runPeakHP) runPeakHP = currentHP;
        if (currentTorqueNm > runPeakTorque) runPeakTorque = currentTorqueNm;

        lastSpeedKmH = currentSpeedKmH;

        // Acceleration Timer State Machine (0-60 km/h)
        switch (state) {
            case ACCEL_IDLE:
                // Auto-arm if stationary and throttle opens > 25%
                if (currentSpeedKmH < 2.0f && tpsPercent > 25.0f) {
                    arm();
                }
                break;

            case ACCEL_ARMED:
                if (currentSpeedKmH >= 3.0f) {
                    state = ACCEL_RUNNING;
                    startTimeMs = now;
                    runPeakHP = 0.0f;
                    runPeakTorque = 0.0f;
                    Serial.println("[AccelDyno] Launch detected! Timer & Dyno logging started.");
                }
                break;

            case ACCEL_RUNNING: {
                elapsedTimeSec = (float)(now - startTimeMs) / 1000.0f;

                // Finished 0-60 km/h
                if (currentSpeedKmH >= 60.0f) {
                    state = ACCEL_FINISHED;
                    lastTime0_60 = elapsedTimeSec;
                    Serial.printf("[AccelDyno] 0-60 km/h: %.2fs! Peak WHP: %.1f HP, Peak TQ: %.1f Nm\n", 
                        lastTime0_60, runPeakHP, runPeakTorque);
                    
                    if (lastTime0_60 < bestTime0_60 && lastTime0_60 > 1.0f) {
                        bestTime0_60 = lastTime0_60;
                        saveBestTime();
                    }
                    if (runPeakHP > peakHorsepower) {
                        peakHorsepower = runPeakHP;
                        saveDynoRecords();
                    }
                    if (runPeakTorque > peakTorqueNm) {
                        peakTorqueNm = runPeakTorque;
                        saveDynoRecords();
                    }
                }
                else if (elapsedTimeSec > 30.0f) {
                    state = ACCEL_FAILED;
                    Serial.println("[AccelDyno] Aborted (Timeout > 30s).");
                }
                else if (elapsedTimeSec > 3.0f && currentSpeedKmH < 4.0f) {
                    state = ACCEL_FAILED;
                    Serial.println("[AccelDyno] Aborted (Vehicle stopped).");
                }
                break;
            }

            case ACCEL_FINISHED:
            case ACCEL_FAILED:
                if (currentSpeedKmH < 2.0f) {
                    state = ACCEL_IDLE;
                }
                break;
        }
    }

    void saveBestTime() {
        Preferences prefs;
        if (prefs.begin("accel_perf", false)) {
            prefs.putFloat("best_0_60", bestTime0_60);
            prefs.end();
        }
    }

    void saveDynoRecords() {
        Preferences prefs;
        if (prefs.begin("accel_perf", false)) {
            prefs.putFloat("peak_hp", peakHorsepower);
            prefs.putFloat("peak_tq", peakTorqueNm);
            prefs.end();
        }
    }

    AccelState getState() const { return state; }
    float getElapsedSec() const { return elapsedTimeSec; }
    float getLastTime0_60() const { return lastTime0_60; }
    float getBestTime0_60() const { return bestTime0_60; }
    float getMaxSpeed() const { return maxSpeedReached; }
    float getCurrentHP() const { return currentHP; }
    float getCurrentTorqueNm() const { return currentTorqueNm; }
    float getPeakHP() const { return (runPeakHP > 0.0f) ? runPeakHP : peakHorsepower; }
    float getPeakTorqueNm() const { return (runPeakTorque > 0.0f) ? runPeakTorque : peakTorqueNm; }

    String getStateString() const {
        switch (state) {
            case ACCEL_IDLE:     return "IDLE";
            case ACCEL_ARMED:    return "ARMED";
            case ACCEL_RUNNING:  return "MEASURING";
            case ACCEL_FINISHED: return "FINISHED";
            case ACCEL_FAILED:   return "ABORTED";
            default:             return "UNKNOWN";
        }
    }

    String toJson() const {
        StaticJsonDocument<384> doc;
        doc["state"] = getStateString();
        doc["elapsed"] = serialized(String(elapsedTimeSec, 2));
        doc["last_0_60"] = serialized(String(lastTime0_60, 2));
        doc["best_0_60"] = (bestTime0_60 > 90.0f) ? serialized(String("0.00")) : serialized(String(bestTime0_60, 2));
        doc["top_speed"] = serialized(String(maxSpeedReached, 1));
        doc["hp"] = serialized(String(currentHP, 1));
        doc["torque_nm"] = serialized(String(currentTorqueNm, 1));
        doc["peak_hp"] = serialized(String((runPeakHP > 0.0f) ? runPeakHP : peakHorsepower, 1));
        doc["peak_torque_nm"] = serialized(String((runPeakTorque > 0.0f) ? runPeakTorque : peakTorqueNm, 1));
        doc["bike_kg"] = serialized(String(bikeWeightKg, 0));
        doc["rider_kg"] = serialized(String(riderWeightKg, 0));
        
        String output;
        serializeJson(doc, output);
        return output;
    }

private:
    AccelerationTimer() 
        : state(ACCEL_IDLE), startTimeMs(0), elapsedTimeSec(0.0f),
          lastTime0_60(0.0f), bestTime0_60(99.99f), maxSpeedReached(0.0f),
          lastSpeedKmH(0.0f), lastUpdateTime(0), currentHP(0.0f), currentTorqueNm(0.0f),
          runPeakHP(0.0f), runPeakTorque(0.0f), peakHorsepower(0.0f), peakTorqueNm(0.0f),
          bikeWeightKg(125.0f), riderWeightKg(70.0f) {}

    AccelState state;
    uint32_t startTimeMs;
    float elapsedTimeSec;
    float lastTime0_60;
    float bestTime0_60;
    float maxSpeedReached;

    // Dyno telemetry
    float lastSpeedKmH;
    uint32_t lastUpdateTime;
    float currentHP;
    float currentTorqueNm;
    float runPeakHP;
    float runPeakTorque;
    float peakHorsepower;
    float peakTorqueNm;
    float bikeWeightKg;
    float riderWeightKg;
};

#endif // ACCELERATION_TIMER_H
