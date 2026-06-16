/**
 * @file relay_service.h
 * @brief Relay Output Service (Optional) - controllable outputs for safety actions.
 * Gracefully disabled if no relay pins configured.
 * Use cases: fuel pump cut on overheat, external alarm, fan control.
 */

#ifndef RELAY_SERVICE_H
#define RELAY_SERVICE_H

#include <Arduino.h>

#define MAX_RELAYS 2

struct RelayConfig {
    int pin;
    String label;
    bool activeLow;     // Most relay modules are active LOW
    bool autoTrigger;   // Allow automatic triggering by system
};

class RelayService {
public:
    RelayService() : relayCount(0) {}

    /**
     * Initialize relay outputs.
     * @param pin1 First relay GPIO (-1 to disable)
     * @param pin2 Second relay GPIO (-1 to disable)
     */
    void init(int pin1 = -1, int pin2 = -1) {
        if (pin1 >= 0) addRelay(pin1, "Relay 1", true, true);
        if (pin2 >= 0) addRelay(pin2, "Relay 2", true, false);

        if (relayCount == 0) {
            Serial.println("[Relay] No relays configured - disabled");
        } else {
            Serial.printf("[Relay] %d relay(s) initialized\n", relayCount);
        }
    }

    bool isAvailable() const { return relayCount > 0; }

    // Manual control
    void setRelay(int index, bool on) {
        if (index < 0 || index >= relayCount) return;
        bool pinState = relays[index].activeLow ? !on : on;
        digitalWrite(relays[index].pin, pinState);
        relayStates[index] = on;
    }

    bool getRelayState(int index) const {
        if (index < 0 || index >= relayCount) return false;
        return relayStates[index];
    }

    // Auto-trigger based on conditions
    void updateAutoTriggers(float temperature, float batteryV, bool overheat) {
        if (relayCount == 0) return;

        // Relay 0: Emergency fuel cut on critical overheat
        if (relays[0].autoTrigger) {
            if (overheat && temperature > 110.0f) {
                setRelay(0, true); // Activate relay (cut fuel pump)
                Serial.println("[Relay] EMERGENCY: Fuel cut activated (overheat)");
            } else if (temperature < 95.0f && relayStates[0]) {
                setRelay(0, false); // Deactivate when cooled down
            }
        }

        // Relay 1: Could be used for fan, alarm, etc.
        // Not auto-triggered by default
    }

    void allOff() {
        for (int i = 0; i < relayCount; i++) {
            setRelay(i, false);
        }
    }

    int getCount() const { return relayCount; }

private:
    RelayConfig relays[MAX_RELAYS];
    bool relayStates[MAX_RELAYS] = {false, false};
    int relayCount;

    void addRelay(int pin, const String& label, bool activeLow, bool autoTrigger) {
        if (relayCount >= MAX_RELAYS) return;
        relays[relayCount] = {pin, label, activeLow, autoTrigger};
        pinMode(pin, OUTPUT);
        // Start with relay OFF
        digitalWrite(pin, activeLow ? HIGH : LOW);
        relayStates[relayCount] = false;
        relayCount++;
        Serial.printf("[Relay] %s on GPIO %d (active %s)\n", 
            label.c_str(), pin, activeLow ? "LOW" : "HIGH");
    }
};

#endif // RELAY_SERVICE_H
