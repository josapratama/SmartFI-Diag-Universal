/**
 * @file actuator_service.h
 * @brief Actuator Active Test Service for Motorcycle Diagnostics (Bi-directional control).
 * Tests Fuel Pump, Injector Solenoid click, Ignition Spark, Radiator Fan, and Tacho sweep.
 */

#ifndef ACTUATOR_SERVICE_H
#define ACTUATOR_SERVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "services/relay_service.h"
#include "services/buzzer_service.h"

enum ActuatorType {
    ACT_NONE = 0,
    ACT_FUEL_PUMP,
    ACT_INJECTOR,
    ACT_IGNITION_COIL,
    ACT_RADIATOR_FAN,
    ACT_TACHO_SWEEP
};

struct ActuatorStatus {
    ActuatorType type;
    String name;
    bool active;
    uint32_t start_time;
    uint32_t duration_ms;
    uint32_t pulse_count;
    uint32_t target_pulses;
    String status_message;
};

class ActuatorService {
public:
    static ActuatorService& getInstance() {
        static ActuatorService instance;
        return instance;
    }

    void init(RelayService* relay = nullptr, BuzzerService* buzzer = nullptr) {
        relayRef = relay;
        buzzerRef = buzzer;
        currentStatus = {ACT_NONE, "None", false, 0, 0, 0, 0, "Idle"};
    }

    bool startTest(ActuatorType type, uint32_t durationMs = 5000, float currentRpm = 0.0f) {
        // Safety lock: Never run actuator test if engine is actively running
        if (currentRpm > 400.0f) {
            currentStatus.status_message = "DITOLAK: Mesin menyala! Matikan mesin sebelum tes aktuator.";
            Serial.println("[Actuator] Rejected: Engine is running!");
            return false;
        }

        stopTest(); // Reset any previous test

        currentStatus.type = type;
        currentStatus.active = true;
        currentStatus.start_time = millis();
        currentStatus.duration_ms = durationMs;
        currentStatus.pulse_count = 0;
        lastPulseTime = millis();

        switch (type) {
            case ACT_FUEL_PUMP:
                currentStatus.name = "Fuel Pump Prime";
                currentStatus.target_pulses = 1;
                if (relayRef && relayRef->isAvailable()) {
                    relayRef->setRelay(0, true);
                }
                if (buzzerRef) buzzerRef->play(BUZZ_CONFIRM);
                currentStatus.status_message = "Pompa bensin aktif selama 5 detik...";
                break;

            case ACT_INJECTOR:
                currentStatus.name = "Injector Click Test";
                currentStatus.target_pulses = 30; // 30 pulses at 15Hz (~2 seconds)
                currentStatus.duration_ms = 2500;
                pulseState = false;
                currentStatus.status_message = "Mengirim 30 pulsa injektor (15Hz)...";
                break;

            case ACT_IGNITION_COIL:
                currentStatus.name = "Ignition Spark Test";
                currentStatus.target_pulses = 20; // 20 sparks at 10Hz (2 seconds)
                currentStatus.duration_ms = 2500;
                pulseState = false;
                currentStatus.status_message = "Menguji loncatan koil pengapian (10Hz)...";
                break;

            case ACT_RADIATOR_FAN:
                currentStatus.name = "Radiator Fan Test";
                currentStatus.target_pulses = 1;
                if (relayRef && relayRef->getCount() > 1) {
                    relayRef->setRelay(1, true);
                }
                if (buzzerRef) buzzerRef->play(BUZZ_CONFIRM);
                currentStatus.status_message = "Kipas radiator aktif selama tes...";
                break;

            case ACT_TACHO_SWEEP:
                currentStatus.name = "Tachometer Gauge Sweep";
                currentStatus.target_pulses = 100;
                currentStatus.duration_ms = 4000;
                currentStatus.status_message = "Melakukan sweep jarum RPM (0 -> Max -> 0)...";
                break;

            default:
                stopTest();
                return false;
        }

        Serial.printf("[Actuator] Started test: %s (Duration: %d ms)\n", currentStatus.name.c_str(), currentStatus.duration_ms);
        return true;
    }

    void stopTest() {
        if (!currentStatus.active) return;

        // Reset hardware relays
        if (relayRef && relayRef->isAvailable()) {
            relayRef->allOff();
        }

        currentStatus.active = false;
        currentStatus.status_message = "Tes selesai.";
        Serial.printf("[Actuator] Test finished: %s\n", currentStatus.name.c_str());
    }

    void update(float currentRpm = 0.0f) {
        if (!currentStatus.active) return;

        // Emergency abort if engine starts
        if (currentRpm > 400.0f) {
            stopTest();
            currentStatus.status_message = "EMERGENCY STOP: Mesin hidup tiba-tiba!";
            return;
        }

        uint32_t now = millis();

        // Check overall duration timeout
        if (now - currentStatus.start_time >= currentStatus.duration_ms) {
            stopTest();
            return;
        }

        // Handle pulsed actuators (Injector & Ignition Coil simulation / clicks)
        if (currentStatus.type == ACT_INJECTOR) {
            // 15Hz = period ~66ms (33ms ON, 33ms OFF)
            if (now - lastPulseTime >= 33) {
                lastPulseTime = now;
                pulseState = !pulseState;
                if (pulseState) {
                    currentStatus.pulse_count++;
                    if (buzzerRef) buzzerRef->play(BUZZ_SHORT_BEEP);
                }
                if (currentStatus.pulse_count >= currentStatus.target_pulses) {
                    stopTest();
                }
            }
        } else if (currentStatus.type == ACT_IGNITION_COIL) {
            // 10Hz = period 100ms (50ms ON, 50ms OFF)
            if (now - lastPulseTime >= 50) {
                lastPulseTime = now;
                pulseState = !pulseState;
                if (pulseState) {
                    currentStatus.pulse_count++;
                    if (buzzerRef) buzzerRef->play(BUZZ_SHORT_BEEP);
                }
                if (currentStatus.pulse_count >= currentStatus.target_pulses) {
                    stopTest();
                }
            }
        }
    }

    float getVirtualTachoSweepRPM() const {
        if (!currentStatus.active || currentStatus.type != ACT_TACHO_SWEEP) return 0.0f;
        uint32_t elapsed = millis() - currentStatus.start_time;
        float progress = (float)elapsed / (float)currentStatus.duration_ms;
        if (progress > 1.0f) progress = 1.0f;
        
        // Triangle wave 0 -> 10,000 -> 0 RPM
        if (progress <= 0.5f) {
            return (progress / 0.5f) * 10000.0f;
        } else {
            return (1.0f - (progress - 0.5f) / 0.5f) * 10000.0f;
        }
    }

    String toJson() const {
        StaticJsonDocument<384> doc;
        doc["active"] = currentStatus.active;
        doc["type"] = (int)currentStatus.type;
        doc["name"] = currentStatus.name;
        doc["message"] = currentStatus.status_message;
        doc["pulses"] = currentStatus.pulse_count;
        doc["target_pulses"] = currentStatus.target_pulses;

        if (currentStatus.active) {
            uint32_t elapsed = millis() - currentStatus.start_time;
            doc["remaining_ms"] = (elapsed < currentStatus.duration_ms) ? (currentStatus.duration_ms - elapsed) : 0;
            doc["progress_pct"] = (int)((float)elapsed / (float)currentStatus.duration_ms * 100.0f);
        } else {
            doc["remaining_ms"] = 0;
            doc["progress_pct"] = 0;
        }

        String output;
        serializeJson(doc, output);
        return output;
    }

    const ActuatorStatus& getStatus() const { return currentStatus; }

private:
    ActuatorService() : relayRef(nullptr), buzzerRef(nullptr), pulseState(false), lastPulseTime(0) {}
    RelayService* relayRef;
    BuzzerService* buzzerRef;
    ActuatorStatus currentStatus;
    bool pulseState;
    uint32_t lastPulseTime;
};

#endif // ACTUATOR_SERVICE_H

