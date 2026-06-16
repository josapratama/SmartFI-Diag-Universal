/**
 * @file driver_base.h
 * @brief Base Driver classes for Analog and Digital inputs.
 * Uses hardware interrupts for non-blocking pulse measurement on ESP32.
 */

#ifndef DRIVER_BASE_H
#define DRIVER_BASE_H

#include <Arduino.h>
#include "config_types.h"

// ==========================================
// ANALOG DRIVER
// ==========================================
class AnalogDriver {
public:
    AnalogDriver(const AnalogChannelConfig& config) : cfg(config), filteredValue(0) {}
    
    void init() {
        pinMode(cfg.pin, INPUT);
    }

    float readRawVoltage() {
        int raw = analogRead(cfg.pin);
        // Standard ESP32 3.3V / 12-bit ADC
        float voltage = (raw * 3.3f / 4095.0f);
        return voltage;
    }

    float read() {
        float voltage = readRawVoltage();
        
        // Guard against division by zero from bad config
        if (cfg.divider_ratio > 0.0f) {
            voltage /= cfg.divider_ratio;
        }
        
        // EMA Filtering
        filteredValue = (cfg.filter_alpha * voltage) + ((1.0f - cfg.filter_alpha) * filteredValue);
        return filteredValue;
    }

    float readConverted() {
        float value = read();
        if (cfg.formula.indexOf("*") != -1) {
            float factor = cfg.formula.substring(cfg.formula.indexOf("*") + 1).toFloat();
            return value * factor;
        }
        return value;
    }

    const AnalogChannelConfig& getConfig() const { return cfg; }
    String getUnits() const { return cfg.units; }

private:
    AnalogChannelConfig cfg;
    float filteredValue;
};

// ==========================================
// INTERRUPT-BASED PULSE CAPTURE
// ==========================================

/**
 * ISR-safe pulse data structure.
 * Shared between ISR and main loop via volatile.
 */
struct PulseISRData {
    volatile uint32_t lastRisingTime;
    volatile uint32_t lastFallingTime;
    volatile uint32_t pulseDuration;    // HIGH pulse width in microseconds
    volatile uint32_t periodDuration;   // Full period (rising-to-rising) in microseconds
    volatile uint32_t pulseCount;
    volatile bool newPulseAvailable;
};

// Global ISR data slots (max 8 digital channels)
static const int MAX_ISR_CHANNELS = 8;
static PulseISRData g_pulseData[MAX_ISR_CHANNELS];
static int g_isrChannelCount = 0;

// ==========================================
// DIGITAL DRIVER (Interrupt-Based)
// ==========================================
class DigitalDriver {
public:
    DigitalDriver(const DigitalChannelConfig& config) 
        : cfg(config), isrSlot(-1), lastValidPulse(0), lastValidPeriod(0), 
          staleTimeout(500000) {} // 500ms timeout = engine stopped
    
    void init() {
        pinMode(cfg.pin, cfg.active_low ? INPUT_PULLUP : INPUT_PULLDOWN);
        
        // Allocate ISR slot and attach interrupt
        if (g_isrChannelCount < MAX_ISR_CHANNELS) {
            isrSlot = g_isrChannelCount++;
            
            // Initialize ISR data
            PulseISRData& d = g_pulseData[isrSlot];
            d.lastRisingTime = 0;
            d.lastFallingTime = 0;
            d.pulseDuration = 0;
            d.periodDuration = 0;
            d.pulseCount = 0;
            d.newPulseAvailable = false;
            
            // Attach interrupt based on slot
            attachInterruptArg(digitalPinToInterrupt(cfg.pin), isrHandler, (void*)(intptr_t)isrSlot, CHANGE);
            
            Serial.printf("[DigitalDriver] Pin %d -> ISR slot %d (interrupt-based)\n", cfg.pin, isrSlot);
        } else {
            Serial.printf("[DigitalDriver] Pin %d -> NO ISR SLOT AVAILABLE (fallback mode)\n", cfg.pin);
        }
    }

    bool read() {
        bool state = digitalRead(cfg.pin);
        return cfg.active_low ? !state : state;
    }

    /**
     * Get pulse duration (HIGH time) in microseconds.
     * Non-blocking: reads from ISR-updated data.
     * Returns 0 if no recent pulse (engine stopped).
     */
    uint32_t getPulseDuration() {
        if (isrSlot < 0) return 0;
        
        PulseISRData& d = g_pulseData[isrSlot];
        
        // Check for stale data (no pulse for timeout period)
        uint32_t now = micros();
        if (d.lastRisingTime > 0 && (now - d.lastRisingTime) > staleTimeout) {
            lastValidPulse = 0;
            lastValidPeriod = 0;
            return 0;
        }
        
        // Read ISR data atomically
        noInterrupts();
        uint32_t pulse = d.pulseDuration;
        uint32_t period = d.periodDuration;
        d.newPulseAvailable = false;
        interrupts();
        
        // Validate: pulse must be reasonable (10us to 100ms)
        if (pulse >= 10 && pulse <= 100000) {
            lastValidPulse = pulse;
        }
        if (period >= 100 && period <= 1000000) {
            lastValidPeriod = period;
        }
        
        return lastValidPulse;
    }

    /**
     * Get full period (rising-to-rising) in microseconds.
     * More accurate for RPM calculation than pulse width alone.
     */
    uint32_t getPeriod() {
        if (isrSlot < 0) return 0;
        
        PulseISRData& d = g_pulseData[isrSlot];
        
        // Check for stale data
        uint32_t now = micros();
        if (d.lastRisingTime > 0 && (now - d.lastRisingTime) > staleTimeout) {
            lastValidPeriod = 0;
            return 0;
        }
        
        noInterrupts();
        uint32_t period = d.periodDuration;
        interrupts();
        
        if (period >= 100 && period <= 1000000) {
            lastValidPeriod = period;
        }
        
        return lastValidPeriod;
    }

    /**
     * Get frequency in Hz. Non-blocking.
     */
    float getFrequency() {
        uint32_t period = getPeriod();
        if (period == 0) return 0;
        return 1000000.0f / (float)period;
    }

    /**
     * Get pulse count since last reset. Useful for MIL blink counting.
     */
    uint32_t getPulseCount() {
        if (isrSlot < 0) return 0;
        noInterrupts();
        uint32_t count = g_pulseData[isrSlot].pulseCount;
        interrupts();
        return count;
    }

    void resetPulseCount() {
        if (isrSlot < 0) return;
        noInterrupts();
        g_pulseData[isrSlot].pulseCount = 0;
        interrupts();
    }

    const DigitalChannelConfig& getConfig() const { return cfg; }

private:
    DigitalChannelConfig cfg;
    int isrSlot;
    uint32_t lastValidPulse;
    uint32_t lastValidPeriod;
    uint32_t staleTimeout;  // Microseconds before considering signal dead

    /**
     * Static ISR handler that dispatches to the correct slot.
     * Kept minimal for IRAM placement.
     */
    static void ARDUINO_ISR_ATTR isrHandler(void* arg) {
        int slot = (int)(intptr_t)arg;
        if (slot < 0 || slot >= MAX_ISR_CHANNELS) return;
        
        uint32_t now = micros();
        volatile PulseISRData& d = g_pulseData[slot];
        
        if (d.lastRisingTime == 0) {
            d.lastRisingTime = now;
            return;
        }
        
        if (d.lastFallingTime <= d.lastRisingTime) {
            // Falling edge
            d.lastFallingTime = now;
            d.pulseDuration = now - d.lastRisingTime;
            d.pulseCount++;
            d.newPulseAvailable = true;
        } else {
            // Rising edge
            d.periodDuration = (now - d.lastRisingTime) + d.pulseDuration;
            d.lastRisingTime = now;
        }
    }
};

#endif // DRIVER_BASE_H
