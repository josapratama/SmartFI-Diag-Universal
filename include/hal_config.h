/**
 * @file hal_config.h
 * @brief Hardware Abstraction Layer configuration for ESP32 Smart FI Diagnostic Tool.
 * Defines pin mappings and hardware-specific constants.
 */

#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. POWER MODULE CONFIG
// ==========================================
#define BATT_MONITOR_PIN    34  // Analog Input
#define BATT_DIVIDER_R1     100000.0f // 100k Ohm
#define BATT_DIVIDER_R2     22000.0f  // 22k Ohm
#define BATT_VREF           3.3f
#define ADC_RESOLUTION      4095.0f

// ==========================================
// 2. ANALOG INPUTS (PROTECTED)
// ==========================================
#define TPS_ADC_PIN         35  // Analog Input (0-3.3V after divider/buffer)
#define ENGINE_TEMP_PIN     32  // Analog Input (NTC divider)

// ==========================================
// 3. ISOLATED DIGITAL INPUTS (OPTICAL/TRANSISTOR)
// ==========================================
#define MIL_SIGNAL_PIN      25  // Pulse/Level from MIL lamp
#define STARTER_SIGNAL_PIN  26  // High when starter button pressed
#define IGNITION_PULSE_PIN  27  // Pulse from Coil/CDI/ECU signal (Freq Input)
#define INJECTOR_PULSE_PIN  14  // Pulse from Injector driver signal (Freq/PWM Input)
#define FUEL_PUMP_PIN       12  // Optional Fuel Pump relay monitor

// ==========================================
// 4. DISPLAY & UI
// ==========================================
#define OLED_SDA            21
#define OLED_SCL            22
#define BUTTON_UP_PIN       33
#define BUTTON_DOWN_PIN     19
#define BUTTON_SELECT_PIN   18
#define BUTTON_BACK_PIN     5
#define BUZZER_PIN          13

// ==========================================
// 5. STORAGE & COMMUNICATION
// ==========================================
#define SD_CS_PIN           4
#define SD_MOSI             23
#define SD_MISO             19
#define SD_SCK              18

// ==========================================
// 6. EXPANSION & ADVANCED
// ==========================================
#define K_LINE_RX           16  // Future ECU communication
#define K_LINE_TX           17

// ==========================================
// SIGNAL CONDITIONING CONSTANTS
// ==========================================
#define ANALOG_EMA_ALPHA    0.1f   // Smoothing factor
#define RPM_SAMPLES         5      // Number of pulses to average for RPM
#define DEBOUNCE_MS         50     // Button debounce time

#endif // HAL_CONFIG_H
