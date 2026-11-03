/**
 * @file acceleration_timer.h
 * @brief Virtual Acceleration & Performance Timer (0-60 km/h).
 * Uses GPS speed or estimated speed from RPM/VSS with automatic start/stop detection.
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
            prefs.end();
        }
    }

    void arm() {
        state = ACCEL_ARMED;
        startTimeMs = 0;
        elapsedTimeSec = 0.0f;
        maxSpeedReached = 0.0f;
        Serial.println("[AccelTimer] Armed. Ready for launch!");
    }

    void reset() {
        state = ACCEL_IDLE;
        startTimeMs = 0;
        elapsedTimeSec = 0.0f;
        maxSpeedReached = 0.0f;
    }

    void update(float currentSpeedKmH, float tpsPercent = 0.0f) {
        if (currentSpeedKmH > maxSpeedReached) {
            maxSpeedReached = currentSpeedKmH;
        }

        switch (state) {
            case ACCEL_IDLE:
                // Auto-arm if stationary and throttle opens
                if (currentSpeedKmH < 2.0f && tpsPercent > 20.0f) {
                    arm();
                }
                break;

            case ACCEL_ARMED:
                // Trigger start when vehicle starts moving
                if (currentSpeedKmH >= 3.0f) {
                    state = ACCEL_RUNNING;
                    startTimeMs = millis();
                    Serial.println("[AccelTimer] Launch detected! Timer started.");
                }
                break;

            case ACCEL_RUNNING: {
                uint32_t now = millis();
                elapsedTimeSec = (float)(now - startTimeMs) / 1000.0f;

                // Finished 0-60 km/h
                if (currentSpeedKmH >= 60.0f) {
                    state = ACCEL_FINISHED;
                    lastTime0_60 = elapsedTimeSec;
                    Serial.printf("[AccelTimer] 0-60 km/h achieved in %.2f seconds!\n", lastTime0_60);
                    
                    if (lastTime0_60 < bestTime0_60 && lastTime0_60 > 1.0f) {
                        bestTime0_60 = lastTime0_60;
                        saveBestTime();
                    }
                }
                // Timeout abort after 30 seconds
                else if (elapsedTimeSec > 30.0f) {
                    state = ACCEL_FAILED;
                    Serial.println("[AccelTimer] Run aborted (Timeout > 30s).");
                }
                // Stopped before hitting target
                else if (elapsedTimeSec > 3.0f && currentSpeedKmH < 5.0f) {
                    state = ACCEL_FAILED;
                    Serial.println("[AccelTimer] Run aborted (Vehicle stopped).");
                }
                break;
            }

            case ACCEL_FINISHED:
            case ACCEL_FAILED:
                // Wait until vehicle comes to complete stop before allowing re-arm
                if (currentSpeedKmH < 2.0f) {
                    state = ACCEL_IDLE;
                }
                break;
        }
    }

    AccelState getState() const { return state; }
    float getElapsedSec() const { return elapsedTimeSec; }
    float getLastTime0_60() const { return lastTime0_60; }
    float getBestTime0_60() const { return bestTime0_60; }
    float getMaxSpeed() const { return maxSpeedReached; }

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
        StaticJsonDocument<256> doc;
        doc["state"] = getStateString();
        doc["elapsed"] = serialized(String(elapsedTimeSec, 2));
        doc["last_0_60"] = serialized(String(lastTime0_60, 2));
        doc["best_0_60"] = (bestTime0_60 > 90.0f) ? 0.0f : serialized(String(bestTime0_60, 2));
        doc["top_speed"] = serialized(String(maxSpeedReached, 1));
        
        String output;
        serializeJson(doc, output);
        return output;
    }

private:
    AccelerationTimer() 
        : state(ACCEL_IDLE), startTimeMs(0), elapsedTimeSec(0.0f),
          lastTime0_60(0.0f), bestTime0_60(99.99f), maxSpeedReached(0.0f) {}

    AccelState state;
    uint32_t startTimeMs;
    float elapsedTimeSec;
    float lastTime0_60;
    float bestTime0_60;
    float maxSpeedReached;

    void saveBestTime() {
        Preferences prefs;
        if (prefs.begin("accel_perf", false)) {
            prefs.putFloat("best_0_60", bestTime0_60);
            prefs.end();
            Serial.printf("[AccelTimer] New personal best saved: %.2fs\n", bestTime0_60);
        }
    }
};

#endif // ACCELERATION_TIMER_H

