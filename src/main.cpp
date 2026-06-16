/**
 * @file main.cpp
 * @brief Main Entry Point for ESP32 Smart FI Diagnostic Tool.
 */

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>
#include <Preferences.h>
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
#include "core/honda_kline_plugin.h"
#include "core/yamaha_kline_plugin.h"
#include "core/kawasaki_kds_plugin.h"
#include "core/suzuki_sds_plugin.h"
#include "services/profile_selector.h"
#include "services/health_score.h"
#include "services/freeze_frame.h"
#include "services/cranking_analyzer.h"
#include "services/dtc_reader.h"
#include "services/diagnostic_wizard.h"
#include "services/fuel_estimator.h"
#include "services/trend_analyzer.h"
#include "services/report_generator.h"
#include "services/dtc_database.h"
#include "services/bluetooth_service.h"
#include "services/ota_service.h"
#include "core/memory_manager.h"
#include "services/preferences_service.h"
#include "services/buzzer_service.h"
#include "services/timing_analyzer.h"
#include "services/o2_sensor_service.h"
#include "services/maintenance_service.h"
#include "services/session_manager.h"
#include "services/gps_service.h"
#include "services/canbus_service.h"
#include "services/relay_service.h"
#include "services/module_status.h"

// ==========================================
// SYSTEM OBJECTS
// ==========================================
HardwareConfig hwConfig;
VehicleProfile currentProfile;

// Core Services
BatteryService batteryService;
TPSService tpsService;
RPMService rpmService;
MILService milService;
StarterService starterService;
TemperatureService temperatureService;
InjectorService injectorService;
WebServerService webServer;

// Diagnostic & Analysis
DiagEngine diagEngine;
UIManager uiManager;
HealthScoreCalculator healthCalc;
CrankingAnalyzer crankAnalyzer;
DTCReader dtcReader;
DiagnosticWizard wizard;
FuelEstimator fuelEstimator;
TrendAnalyzer trendAnalyzer;
ReportGenerator reportGen;
ProfileSelector profileSelector;
BluetoothService btService;
OTAService otaService;

// New services
BuzzerService buzzer;
TimingAnalyzer timingAnalyzer;
O2SensorService o2Service;
MaintenanceService maintenanceService;
SessionManager sessionManager;
GPSService gpsService;
CANBusService canBusService;
RelayService relayService;

// ==========================================
// SYSTEM STATES
// ==========================================
enum SystemState {
    STATE_INIT,
    STATE_PROFILE_SELECT,
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
void handleAdvancedFeatures();
void handleWebRequests();
void handleBluetooth();

// Helper for web server module status endpoint
String getModuleStatusJson() {
    return ModuleStatusTracker::getInstance().toJson();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== Smart FI Diagnostic Tool ===");
    Serial.println("System Starting...");

    // 1. Initialize SD Card & Logging
    if (!LoggingService::getInstance().init(4)) {
        Serial.println("SD Card Failed! HALTING.");
        while (1);
    }
    LoggingService::getInstance().startNewSession();

    // 2. Load Hardware Configuration
    // ============================================================
    // PILIH SALAH SATU BOARD (comment/uncomment):
    // ============================================================
    const char* HW_CONFIG = "/configs/hardware/diagnostic_shield_v1_esp32.json";    // ESP32 Original
    // const char* HW_CONFIG = "/configs/hardware/diagnostic_shield_v1_esp32s3.json"; // ESP32-S3 N16R8
    // ============================================================
    
    if (!ConfigLoader::loadHardwareConfig(HW_CONFIG, hwConfig)) {
        Serial.println("Failed to load Hardware Config! HALTING.");
        Serial.println("File: " + String(HW_CONFIG));
        while (1);
    }
    HardwareRegistry::getInstance().setConfig(hwConfig);

    // 3. Profile Selection via OLED
    profileSelector.scanProfiles();
    
    String selectedProfile = "";
    if (profileSelector.getProfiles().size() > 1) {
        // Initialize display early for profile selection
        Wire.begin(hwConfig.i2c_display.sda, hwConfig.i2c_display.scl);
        Adafruit_SSD1306 tempDisplay(128, 64, &Wire, -1);
        if (tempDisplay.begin(SSD1306_SWITCHCAPVCC, hwConfig.i2c_display.address)) {
            currentState = STATE_PROFILE_SELECT;
            selectedProfile = profileSelector.showSelectionUI(
                &tempDisplay, 
                hwConfig.buttons.up, 
                hwConfig.buttons.down, 
                hwConfig.buttons.select
            );
        }
    }

    // Load selected or default profile (check NVS for last used)
    if (selectedProfile.isEmpty()) {
        String savedProfile = PreferencesService::getInstance().getLastProfile();
        if (!savedProfile.isEmpty()) {
            selectedProfile = savedProfile;
            Serial.println("Using saved profile: " + savedProfile);
        } else {
            selectedProfile = "/configs/vehicles/model_honda_beat_2014.json";
        }
    }
    
    if (!ConfigLoader::loadVehicleProfile(selectedProfile.c_str(), currentProfile)) {
        Serial.println("Failed to load Vehicle Profile! HALTING.");
        while (1);
    }
    Serial.println("Profile Loaded: " + currentProfile.name);

    // 4. Initialize Core Manager
    ChannelManager::getInstance().init(hwConfig);

    // 5. Initialize Sensor Services
    batteryService.init();
    tpsService.init();
    rpmService.init();
    milService.init();
    starterService.init();
    temperatureService.init();
    injectorService.init();

    // 6. Register Protocol Plugins (all brands share K-Line pins)
    ProtocolManager::getInstance().registerPlugin("honda", 
        std::make_shared<HondaKLinePlugin>(16, 17));
    ProtocolManager::getInstance().registerPlugin("yamaha", 
        std::make_shared<YamahaKLinePlugin>(16, 17));
    ProtocolManager::getInstance().registerPlugin("kawasaki", 
        std::make_shared<KawasakiKDSPlugin>(16, 17));
    ProtocolManager::getInstance().registerPlugin("suzuki", 
        std::make_shared<SuzukiSDSPlugin>(16, 17));

    // 7. Initialize DTC Database (from SD or built-in)
    DTCDatabase::getInstance().load(currentProfile.brand);

    // 8. Initialize Fuel Estimator
    fuelEstimator.init();

    // 9. Initialize WiFi & Web Dashboard
    WiFi.softAP(hwConfig.project_name.c_str(), "smartfi123");
    Serial.println("WiFi AP: " + hwConfig.project_name);
    Serial.println("Password: smartfi123");
    Serial.println("Dashboard: http://192.168.4.1");
    webServer.init(hwConfig);

    // OTA Firmware Update (register before server.begin)
    otaService.init(webServer.getServer());
    
    // Start web server
    webServer.begin();

    // Set wizard symptoms list
    webServer.setWizardSymptoms(wizard.getSymptomListJson());

    // Set profile info
    StaticJsonDocument<256> profileDoc;
    profileDoc["name"] = currentProfile.name;
    profileDoc["brand"] = currentProfile.brand;
    profileDoc["family"] = currentProfile.family;
    profileDoc["tier"] = currentProfile.tier;
    String profileJson;
    serializeJson(profileDoc, profileJson);
    webServer.setProfileInfo(profileJson);

    // 10. Initialize Bluetooth
    btService.init("SmartFI-" + currentProfile.brand);

    // 11. Initialize UI
    uiManager.init(hwConfig);

    // 12. Initialize Memory Manager
    MemoryManager::getInstance().init();

    // 13. Initialize Preferences (NVS) & Auto-load last profile
    PreferencesService::getInstance().init();

    // 14. Initialize Buzzer
    buzzer.init(hwConfig.buzzer_pin);
    buzzer.play(BUZZ_STARTUP_OK);

    // 15. Initialize O2 Sensor (optional - only if CH_O2 configured)
    o2Service.init();

    // 16. Initialize Maintenance Service
    maintenanceService.init();

    // 17. Start Session for current vehicle
    sessionManager.startSession(currentProfile);

    // 18. Initialize GPS (from hardware config JSON)
    if (hwConfig.optional.gps.enabled) {
        gpsService.init(hwConfig.optional.gps.rx_pin, hwConfig.optional.gps.tx_pin);
    } else {
        Serial.println("[GPS] Disabled in config (set enabled=true in JSON to activate)");
    }

    // 19. Initialize CAN Bus (from hardware config JSON)
    if (hwConfig.optional.canbus.enabled) {
        canBusService.init(hwConfig.optional.canbus.cs_pin, hwConfig.optional.canbus.speed);
    } else {
        Serial.println("[CAN] Disabled in config (set enabled=true in JSON to activate)");
    }

    // 20. Initialize Relay outputs (from hardware config JSON)
    if (hwConfig.optional.relay.enabled) {
        relayService.init(hwConfig.optional.relay.pin_1, hwConfig.optional.relay.pin_2);
    } else {
        Serial.println("[Relay] Disabled in config (set enabled=true in JSON to activate)");
    }

    // 21. Register module statuses for UI notifications
    auto& modStatus = ModuleStatusTracker::getInstance();
    
    // Core modules (always expected)
    modStatus.setStatus("SD Card", MOD_CONNECTED, "OK");
    modStatus.setStatus("OLED", MOD_CONNECTED, "128x64");
    modStatus.setStatus("WiFi", MOD_CONNECTED, "192.168.4.1");
    
    // Optional modules
    if (hwConfig.optional.gps.enabled) {
        modStatus.setStatus("GPS", gpsService.isAvailable() ? MOD_CONNECTED : MOD_NOT_DETECTED,
            gpsService.isAvailable() ? "OK" : "Modul tidak merespon. Cek kabel RX.");
    } else {
        modStatus.setStatus("GPS", MOD_DISABLED, "Set enabled=true di JSON");
    }

    if (hwConfig.optional.canbus.enabled) {
        modStatus.setStatus("CAN Bus", canBusService.isAvailable() ? MOD_CONNECTED : MOD_NOT_DETECTED,
            canBusService.isAvailable() ? "MCP2515 OK" : "MCP2515 tidak terdeteksi. Cek SPI & CS pin.");
    } else {
        modStatus.setStatus("CAN Bus", MOD_DISABLED, "Set enabled=true di JSON");
    }

    if (hwConfig.optional.relay.enabled) {
        modStatus.setStatus("Relay", relayService.isAvailable() ? MOD_CONNECTED : MOD_NOT_DETECTED, "");
    } else {
        modStatus.setStatus("Relay", MOD_DISABLED, "Set enabled=true di JSON");
    }

    modStatus.setStatus("Bluetooth", MOD_CONNECTED, "");
    modStatus.setStatus("K-Line ECU", MOD_DISABLED, "Menunggu koneksi ECU");

    // Print module summary
    Serial.println("[Modules] " + modStatus.getOLEDSummary());
    if (modStatus.hasWarnings()) {
        Serial.println("[Modules] PERINGATAN:");
        Serial.print(modStatus.getNotifications());
        buzzer.play(BUZZ_DOUBLE_BEEP); // Warning beep for module issues
    }

    // Save selected profile to NVS for next boot
    PreferencesService::getInstance().saveLastProfile(selectedProfile);

    currentState = STATE_IDLE;
    Serial.println("=== System Ready ===");
}

void loop() {
    static uint32_t lastFastTask = 0;
    static uint32_t lastSlowTask = 0;
    static uint32_t lastTrendTask = 0;
    static uint32_t lastBTTask = 0;
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
        // Update UI with extra data
        uiManager.setFuelRate(fuelEstimator.getCurrentRate());
        uiManager.setMaintenanceAlert(maintenanceService.hasOverdueItems());

        broadcastTelemetry();
        handleLogging();
        handleAdvancedFeatures();
        handleWebRequests();
        
        // Update Protocol Manager for Tier 4 (ECU communication)
        ProtocolManager::getInstance().update(currentProfile);
        
        lastSlowTask = now;
    }

    // Trend recording (1Hz)
    if (now - lastTrendTask >= 1000) {
        trendAnalyzer.record(
            batteryService.getVoltage(),
            rpmService.getRPM(),
            tpsService.getPercent(),
            temperatureService.getTemperature()
        );
        lastTrendTask = now;
    }

    // Bluetooth task (500ms)
    if (now - lastBTTask >= 500) {
        handleBluetooth();
        lastBTTask = now;
    }

    // Memory monitoring (10s)
    static uint32_t lastMemCheck = 0;
    if (now - lastMemCheck >= 10000) {
        MemoryManager::getInstance().update();
        lastMemCheck = now;
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

    // Fuel estimator
    fuelEstimator.update(injectorService.getPulseWidth(), rpmService.getRPM());

    // Cranking analyzer
    crankAnalyzer.update(currentProfile, starterService.getCrankingStatus(), 
                         batteryService.getVoltage(), rpmService.getRPM());

    // Timing analyzer (uses ignition channel data)
    auto ignChan = ChannelManager::getInstance().getDigital("CH_IGN");
    if (ignChan) {
        timingAnalyzer.update(ignChan->getPeriod(), ignChan->getPulseDuration(), rpmService.getRPM());
    }

    // O2 sensor (optional)
    o2Service.update();

    // Maintenance (track running hours)
    maintenanceService.update(rpmService.getRPM());

    // GPS (optional)
    gpsService.update();

    // CAN Bus (optional)
    canBusService.update();

    // Buzzer (non-blocking pattern update)
    buzzer.update();

    // Session tracking
    sessionManager.update();
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

    // Auto freeze frame on MIL activation
    static bool lastMIL = false;
    if (milService.getStatus() && !lastMIL) {
        FreezeFrameService::getInstance().capture(
            "MIL_ON",
            batteryService.getVoltage(),
            tpsService.getVoltage(),
            tpsService.getPercent(),
            rpmService.getRPM(),
            temperatureService.getTemperature(),
            rpmService.getVariance(),
            true, milService.getBlinkCode(),
            injectorService.getPulseWidth(),
            currentProfile.name
        );
        buzzer.play(BUZZ_TRIPLE_BEEP); // Alert: new DTC
        sessionManager.recordDTC(String(milService.getBlinkCode()));
    }
    lastMIL = milService.getStatus();

    // Buzzer alerts for critical conditions
    if (temperatureService.getTemperature() > 105.0f) {
        buzzer.play(BUZZ_SOS); // Critical overheat
    } else if (batteryService.getVoltage() < 10.5f && batteryService.getVoltage() > 5.0f) {
        buzzer.play(BUZZ_LONG_BEEP); // Battery critical
    }

    // Relay auto-triggers
    relayService.updateAutoTriggers(
        temperatureService.getTemperature(),
        batteryService.getVoltage(),
        temperatureService.getTemperature() > 105.0f
    );
}

void handleAdvancedFeatures() {
    // Health Score calculation
    HealthInput hInput;
    hInput.battery_v = batteryService.getVoltage();
    hInput.tps_v = tpsService.getVoltage();
    hInput.rpm = rpmService.getRPM();
    hInput.temperature = temperatureService.getTemperature();
    hInput.rpm_variance = rpmService.getVariance();
    hInput.mil_active = milService.getStatus();
    hInput.tps_out_of_range = diagEngine.getStatus().tps_out_of_range;
    hInput.idle_unstable = diagEngine.getStatus().idle_unstable;
    hInput.charging_failure = diagEngine.getStatus().charging_failure;
    hInput.injector_pw_us = injectorService.getPulseWidth();

    HealthReport healthReport = healthCalc.calculate(currentProfile, hInput);

    // Update web server with health data (every 500ms to reduce heap pressure)
    static uint32_t lastHealthUpdate = 0;
    if (millis() - lastHealthUpdate >= 500) {
        // Use StaticJsonDocument to avoid heap fragmentation
        StaticJsonDocument<384> healthDoc;
        healthDoc["score"] = serialized(String(healthReport.overall_score, 1));
        healthDoc["summary"] = healthReport.summary;
        healthDoc["batt_score"] = serialized(String(healthReport.battery_score, 0));
        healthDoc["tps_score"] = serialized(String(healthReport.tps_score, 0));
        healthDoc["idle_score"] = serialized(String(healthReport.idle_score, 0));
        healthDoc["charge_score"] = serialized(String(healthReport.charging_score, 0));
        healthDoc["thermal_score"] = serialized(String(healthReport.thermal_score, 0));
        
        // Serialize to static buffer then copy to String (one allocation)
        size_t len = serializeJson(healthDoc, JsonBuffers::healthBuf, sizeof(JsonBuffers::healthBuf));
        String healthJson(JsonBuffers::healthBuf, len);
        webServer.setHealthData(healthReport.overall_score, healthJson);
        lastHealthUpdate = millis();
    }

    // Fuel data (lightweight, OK at 100ms)
    webServer.setFuelData(fuelEstimator.getCurrentRate(), fuelEstimator.getTotalCC());

    // Freeze frame data (only update when frame count changes)
    static size_t lastFrameCount = 0;
    size_t currentFrameCount = FreezeFrameService::getInstance().getFrames().size();
    if (currentFrameCount != lastFrameCount) {
        webServer.setFreezeData(FreezeFrameService::getInstance().toJson());
        lastFrameCount = currentFrameCount;
    }

    // Trend data (update every 5 seconds to save CPU)
    static uint32_t lastTrendUpdate = 0;
    if (millis() - lastTrendUpdate >= 5000) {
        webServer.setTrendData(trendAnalyzer.toJson());
        lastTrendUpdate = millis();
    }

    // Cranking events (only update when event count changes)
    static size_t lastCrankCount = 0;
    const auto& crankEvents = crankAnalyzer.getEvents();
    if (crankEvents.size() != lastCrankCount) {
        // Use StaticJsonDocument - max 10 events * ~150 bytes = ~1500 bytes
        StaticJsonDocument<2048> crankDoc;
        JsonArray arr = crankDoc.to<JsonArray>();
        for (const auto& ev : crankEvents) {
            JsonObject obj = arr.createNestedObject();
            obj["duration_ms"] = ev.duration_ms;
            obj["min_v"] = serialized(String(ev.min_voltage, 2));
            obj["max_v"] = serialized(String(ev.max_voltage, 2));
            obj["drop"] = serialized(String(ev.voltage_drop, 2));
            obj["started"] = ev.engine_started;
            obj["verdict"] = ev.verdict;
        }
        String crankJson;
        serializeJson(crankDoc, crankJson);
        webServer.setCrankData(crankJson);
        lastCrankCount = crankEvents.size();
    }

    // DTC reading (every 10 seconds if ECU connected)
    static uint32_t lastDTCRead = 0;
    if (millis() - lastDTCRead >= 10000 && dtcReader.isECUReady(currentProfile)) {
        DTCResult result = dtcReader.readDTCs(currentProfile);
        if (result.success) {
            StaticJsonDocument<2048> dtcDoc;
            JsonArray arr = dtcDoc.to<JsonArray>();
            for (const auto& dtc : result.codes) {
                JsonObject obj = arr.createNestedObject();
                obj["code"] = dtc.code;
                obj["desc"] = dtc.description;
                String fix = DTCDatabase::getInstance().getSuggestedFix(dtc.code);
                if (!fix.isEmpty()) obj["fix"] = fix;
            }
            String dtcJson;
            serializeJson(dtcDoc, dtcJson);
            webServer.setDTCData(dtcJson);

            // Auto freeze frame on new DTC
            if (!result.codes.empty()) {
                FreezeFrameService::getInstance().capture(
                    "DTC_" + result.codes[0].code,
                    batteryService.getVoltage(),
                    tpsService.getVoltage(),
                    tpsService.getPercent(),
                    rpmService.getRPM(),
                    temperatureService.getTemperature(),
                    rpmService.getVariance(),
                    milService.getStatus(), milService.getBlinkCode(),
                    injectorService.getPulseWidth(),
                    currentProfile.name
                );
            }
        }
        lastDTCRead = millis();
    }
}

void handleWebRequests() {
    // Report generation
    if (webServer.isReportRequested()) {
        HealthInput hInput;
        hInput.battery_v = batteryService.getVoltage();
        hInput.tps_v = tpsService.getVoltage();
        hInput.rpm = rpmService.getRPM();
        hInput.temperature = temperatureService.getTemperature();
        hInput.rpm_variance = rpmService.getVariance();
        hInput.mil_active = milService.getStatus();
        hInput.tps_out_of_range = diagEngine.getStatus().tps_out_of_range;
        hInput.idle_unstable = diagEngine.getStatus().idle_unstable;
        hInput.charging_failure = diagEngine.getStatus().charging_failure;
        hInput.injector_pw_us = injectorService.getPulseWidth();
        HealthReport healthReport = healthCalc.calculate(currentProfile, hInput);

        DTCResult dtcResult = dtcReader.getLastResult();
        String reportFile = reportGen.generate(
            currentProfile,
            healthReport,
            dtcResult.codes,
            FreezeFrameService::getInstance().getFrames(),
            crankAnalyzer.getEvents(),
            batteryService.getVoltage(),
            rpmService.getRPM(),
            temperatureService.getTemperature()
        );
        webServer.setLastReport(reportFile);
    }

    // DTC Clear
    if (webServer.isDTCClearRequested()) {
        dtcReader.clearDTCs(currentProfile);
        webServer.setDTCData("[]");
    }

    // Manual Freeze Frame
    if (webServer.isManualFreezeRequested()) {
        FreezeFrameService::getInstance().capture(
            "MANUAL",
            batteryService.getVoltage(),
            tpsService.getVoltage(),
            tpsService.getPercent(),
            rpmService.getRPM(),
            temperatureService.getTemperature(),
            rpmService.getVariance(),
            milService.getStatus(), milService.getBlinkCode(),
            injectorService.getPulseWidth(),
            currentProfile.name
        );
    }

    // Wizard Analysis
    if (webServer.isWizardAnalyzeRequested()) {
        SensorSnapshot snap;
        snap.battery_v = batteryService.getVoltage();
        snap.tps_v = tpsService.getVoltage();
        snap.tps_pct = tpsService.getPercent();
        snap.rpm = rpmService.getRPM();
        snap.rpm_variance = rpmService.getVariance();
        snap.temperature = temperatureService.getTemperature();
        snap.mil_active = milService.getStatus();
        snap.mil_code = milService.getBlinkCode();
        snap.injector_pw_us = injectorService.getPulseWidth();

        WizardResult result = wizard.analyze(webServer.getWizardSymptomId(), currentProfile, snap);
        webServer.setWizardResult(wizard.resultToJson(result));
    }

    // Fuel Reset
    if (webServer.isFuelResetRequested()) {
        fuelEstimator.resetTrip();
    }
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

void handleBluetooth() {
    if (!btService.isConnected()) return;

    // Send telemetry
    HealthInput hInput;
    hInput.battery_v = batteryService.getVoltage();
    hInput.rpm = rpmService.getRPM();
    hInput.temperature = temperatureService.getTemperature();
    hInput.rpm_variance = rpmService.getVariance();
    hInput.mil_active = milService.getStatus();
    hInput.tps_v = tpsService.getVoltage();
    hInput.tps_out_of_range = diagEngine.getStatus().tps_out_of_range;
    hInput.idle_unstable = diagEngine.getStatus().idle_unstable;
    hInput.charging_failure = diagEngine.getStatus().charging_failure;
    hInput.injector_pw_us = injectorService.getPulseWidth();
    HealthReport hr = healthCalc.calculate(currentProfile, hInput);

    btService.sendTelemetry(
        batteryService.getVoltage(),
        rpmService.getRPM(),
        tpsService.getPercent(),
        temperatureService.getTemperature(),
        hr.overall_score,
        fuelEstimator.getCurrentRate()
    );

    // Check for incoming commands
    String cmd = btService.checkIncoming();
    if (cmd == "DTC_READ") {
        DTCResult result = dtcReader.readDTCs(currentProfile);
        if (result.success) {
            btService.sendDTCs(result.codes);
        }
    } else if (cmd == "DTC_CLEAR") {
        if (dtcReader.clearDTCs(currentProfile)) {
            btService.sendAlert("DTC Cleared", "info");
        } else {
            btService.sendAlert("DTC Clear Failed", "error");
        }
    } else if (cmd == "FREEZE") {
        FreezeFrameService::getInstance().capture(
            "BT_MANUAL",
            batteryService.getVoltage(),
            tpsService.getVoltage(),
            tpsService.getPercent(),
            rpmService.getRPM(),
            temperatureService.getTemperature(),
            rpmService.getVariance(),
            milService.getStatus(), milService.getBlinkCode(),
            injectorService.getPulseWidth(),
            currentProfile.name
        );
        btService.sendAlert("Freeze Frame Captured", "info");
    } else if (cmd == "FUEL_RESET") {
        fuelEstimator.resetTrip();
        btService.sendAlert("Fuel Trip Reset", "info");
    }
}
