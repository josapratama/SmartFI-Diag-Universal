/**
 * @file main.cpp
 * @brief Main Entry Point for ESP32 Smart FI Diagnostic Tool.
 */

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>
#include "core/config_loader.h"
#include "core/channel_manager.h"
#include "core/capability_manager.h"
#include "services/battery_service.h"
#include "services/tps_service.h"
#include "services/rpm_service.h"
#include "services/mil_service.h"
#include "services/starter_service.h"
#include "services/web_server_service.h"
#include "services/diag_engine.h"
#include "app/ui_manager.h"
#include "services/temperature_service.h"
#include "services/injector_service.h"
#include "services/logging_service.h"
#include "core/protocol_manager.h"
#include "core/hardware_registry.h"

// ==========================================
// SYSTEM OBJECTS
// ==========================================
HardwareConfig hwConfig;
VehicleProfile currentProfile;

// Services
BatteryService batteryService;
TPSService tpsService;
RPMService rpmService;
MILService milService;
StarterService starterService;
TemperatureService temperatureService;
InjectorService injectorService;
WebServerService webServer;

// Application Logic
DiagEngine diagEngine;
UIManager uiManager;

// ==========================================
// SYSTEM STATES
// ==========================================
enum SystemState {
    STATE_INIT,
    STATE_IDLE,
    STATE_CRANKING,
    STATE_RUNNING,
    STATE_STALL,
    STATE_FAULT
};

SystemState currentState = STATE_INIT;

// ==========================================
// FUNCTION PROTOTYPES
// ==========================================
void updateServices();
void handleDiagnostics();
void handleLogging();
void broadcastTelemetry();

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("System Starting...");

    // 1. Initialize SD Card & Logging
    if (!LoggingService::getInstance().init(4)) {
        Serial.println("SD Card Failed! HALTING.");
        while (1);
    }
    LoggingService::getInstance().startNewSession();

    // 2. Load Configurations
    if (!ConfigLoader::loadHardwareConfig("/configs/hardware/diagnostic_shield_v1.json", hwConfig)) {
        Serial.println("Failed to load Hardware Config! HALTING.");
        while (1);
    }
    HardwareRegistry::getInstance().setConfig(hwConfig);

    if (!ConfigLoader::loadVehicleProfile("/configs/vehicles/model_honda_beat_2014.json", currentProfile)) {
        Serial.println("Failed to load Vehicle Profile! HALTING.");
        while (1);
    }
    Serial.println("Configs Loaded Successfully.");

    // 3. Initialize Core Manager
    ChannelManager::getInstance().init(hwConfig);

    // 4. Initialize Services
    batteryService.init();
    tpsService.init();
    rpmService.init();
    milService.init();
    starterService.init();
    temperatureService.init();
    injectorService.init();

    // 5. Initialize WiFi & Web Dashboard
    WiFi.softAP(hwConfig.project_name.c_str(), "smartfi123");
    webServer.init(hwConfig);

    // 6. Initialize UI
    uiManager.init(hwConfig);

    currentState = STATE_IDLE;
    Serial.println("System Ready.");
}

void loop() {
    static uint32_t lastFastTask = 0;
    static uint32_t lastSlowTask = 0;
    uint32_t now = millis();

    // Fast Task (10ms): High-speed sensor update
    if (now - lastFastTask >= 10) {
        updateServices();
        lastFastTask = now;
    }

    // Slow Task (100ms): Diagnostics, UI, Telemetry
    if (now - lastSlowTask >= 100) {
        handleDiagnostics();
        uiManager.update(
            batteryService.getVoltage(),
            tpsService.getVoltage(),
            rpmService.getRPM(),
            temperatureService.getTemperature()
        );
        broadcastTelemetry();
        handleLogging();
        lastSlowTask = now;
    }
}

void updateServices() {
    batteryService.update(currentProfile);
    if (CapabilityManager::isFeatureAvailable("tps_monitor", currentProfile)) tpsService.update(currentProfile);
    if (CapabilityManager::isFeatureAvailable("rpm_monitor", currentProfile)) rpmService.update(currentProfile);
    if (CapabilityManager::isFeatureAvailable("mil_reader", currentProfile)) milService.update(currentProfile);
    if (CapabilityManager::isFeatureAvailable("temp_monitor", currentProfile)) temperatureService.update(currentProfile);
    if (CapabilityManager::isFeatureAvailable("starter_analyzer", currentProfile)) starterService.update(currentProfile, batteryService.getVoltage());
    if (CapabilityManager::isFeatureAvailable("injector_pulse", currentProfile)) injectorService.update(currentProfile);
    
    // Update Protocol Manager for Tier 4
    ProtocolManager::getInstance().update(currentProfile);
}

void handleDiagnostics() {
    diagEngine.update(
        currentProfile,
        batteryService.getVoltage(),
        tpsService.getVoltage(),
        rpmService.getRPM(),
        temperatureService.getTemperature(),
        milService.getStatus()
    );
}

void broadcastTelemetry() {
    webServer.broadcastLiveData(
        batteryService.getVoltage(),
        rpmService.getRPM(),
        tpsService.getPercent(),
        temperatureService.getTemperature()
    );
}

void handleLogging() {
    static uint32_t lastLog = 0;
    if (millis() - lastLog >= 1000) {
        LoggingService::getInstance().logData(
            batteryService.getVoltage(),
            tpsService.getVoltage(),
            rpmService.getRPM(),
            temperatureService.getTemperature(),
            milService.getStatus()
        );
        lastLog = millis();
    }
}
