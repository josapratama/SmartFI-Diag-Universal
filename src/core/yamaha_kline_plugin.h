/**
 * @file yamaha_kline_plugin.h
 * @brief Implementation of Yamaha ISO 9141 K-Line Protocol (Tier 4).
 * 
 * Yamaha FI motorcycles (Mio Z, R2, etc.) use a variant of ISO 9141
 * K-Line protocol for ECU diagnostics. The communication uses:
 * - Baud rate: 9600 bps
 * - Init sequence: slow init (bit-bang) with target address 0x7A
 * - Data format: 8N1
 * 
 * DTC retrieval uses Yamaha-specific command frames.
 */

#ifndef YAMAHA_KLINE_PLUGIN_H
#define YAMAHA_KLINE_PLUGIN_H

#include "core/protocol_plugin.h"
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <map>

class YamahaKLinePlugin : public ProtocolPlugin {
public:
    YamahaKLinePlugin(int rxPin, int txPin, HardwareSerial& serial = Serial2)
        : _rxPin(rxPin), _txPin(txPin), _serial(serial), _connected(false) {}

    // ==========================================
    // Core ECU Operations
    // ==========================================

    bool connect() override {
        Serial.println("[Yamaha] Initializing K-Line Protocol...");
        
        // Configure serial port
        _serial.begin(9600, SERIAL_8N1, _rxPin, _txPin);
        delay(100);

        // ISO 9141 Slow Init (5-baud init with target address 0x7A)
        if (!slowInit(YAMAHA_ECU_ADDRESS)) {
            Serial.println("[Yamaha] Slow init FAILED.");
            return false;
        }

        // Wait for ECU keyword bytes
        if (!waitForKeywordBytes()) {
            Serial.println("[Yamaha] No keyword response from ECU.");
            return false;
        }

        // Send complement of keyword byte 2 (handshake)
        if (!completeHandshake()) {
            Serial.println("[Yamaha] Handshake FAILED.");
            return false;
        }

        _connected = true;
        Serial.println("[Yamaha] ECU Connected Successfully!");
        return true;
    }

    void disconnect() override {
        _connected = false;
        _serial.end();
        Serial.println("[Yamaha] Disconnected.");
    }

    bool isConnected() override {
        return _connected;
    }

    // ==========================================
    // Data Retrieval
    // ==========================================

    std::vector<DTC> readDTCs() override {
        std::vector<DTC> dtcs;
        if (!_connected) return dtcs;

        Serial.println("[Yamaha] Reading DTCs...");

        // Send DTC read command
        uint8_t cmd[] = {0x72, 0x05, 0x00, 0x73, 0x16};  // Yamaha DTC request frame
        sendFrame(cmd, sizeof(cmd));

        // Read response
        uint8_t response[32];
        int len = readFrame(response, sizeof(response), 500);

        if (len > 4) {
            // Parse DTC codes from response
            // Yamaha format: [header][length][mode][dtc_count][dtc1][dtc2]...[checksum]
            int dtcCount = response[3];
            for (int i = 0; i < dtcCount && (4 + i) < len - 1; i++) {
                uint8_t code = response[4 + i];
                DTC dtc;
                dtc.code = String(code);
                dtc.description = getDTCDescription(code);
                dtc.is_active = true;
                dtcs.push_back(dtc);
            }
        }

        Serial.printf("[Yamaha] Found %d DTC(s)\n", dtcs.size());
        return dtcs;
    }

    bool clearDTCs() override {
        if (!_connected) return false;

        Serial.println("[Yamaha] Clearing DTCs...");

        // Send DTC clear command
        uint8_t cmd[] = {0x72, 0x05, 0x00, 0x60, 0x29};  // Yamaha DTC clear frame
        sendFrame(cmd, sizeof(cmd));

        // Wait for ACK
        uint8_t response[16];
        int len = readFrame(response, sizeof(response), 1000);

        bool success = (len > 2 && response[2] == 0x00);
        Serial.printf("[Yamaha] Clear DTCs: %s\n", success ? "OK" : "FAILED");
        return success;
    }

    float getLiveParam(const String& param_name) override {
        if (!_connected) return 0.0f;

        // Send live data request for specific parameter
        uint8_t paramId = getParamId(param_name);
        if (paramId == 0xFF) return 0.0f;

        uint8_t cmd[] = {0x72, 0x06, 0x00, 0x61, paramId, 0x00};
        cmd[5] = calculateChecksum(cmd, 5);
        sendFrame(cmd, sizeof(cmd));

        uint8_t response[16];
        int len = readFrame(response, sizeof(response), 200);

        if (len > 4) {
            return decodeParam(param_name, response[4], (len > 5) ? response[5] : 0);
        }

        return 0.0f;
    }

    // ==========================================
    // Metadata
    // ==========================================

    String getProtocolName() override { 
        return "Yamaha ISO 9141 K-Line"; 
    }

    bool supportsFeature(const String& feature) override {
        return (feature == "read_dtc" || 
                feature == "clear_dtc" || 
                feature == "live_data" ||
                feature == "ecu_id");
    }

private:
    // ==========================================
    // CONSTANTS
    // ==========================================
    static const uint8_t YAMAHA_ECU_ADDRESS = 0x7A;
    static const int SLOW_INIT_BIT_DURATION_MS = 200;  // 5 baud = 200ms per bit
    static const int RESPONSE_TIMEOUT_MS = 2000;

    // ==========================================
    // PRIVATE MEMBERS
    // ==========================================
    int _rxPin;
    int _txPin;
    HardwareSerial& _serial;
    bool _connected;
    uint8_t _keyByte1 = 0;
    uint8_t _keyByte2 = 0;

    // ==========================================
    // ISO 9141 SLOW INIT (5-baud)
    // ==========================================
    bool slowInit(uint8_t address) {
        // Pull K-Line low for start bit, then send address at 5 baud
        _serial.end();
        pinMode(_txPin, OUTPUT);

        // Start bit (LOW)
        digitalWrite(_txPin, LOW);
        delay(SLOW_INIT_BIT_DURATION_MS);

        // Send address bits LSB first
        for (int i = 0; i < 8; i++) {
            digitalWrite(_txPin, (address >> i) & 0x01 ? HIGH : LOW);
            delay(SLOW_INIT_BIT_DURATION_MS);
        }

        // Stop bit (HIGH)
        digitalWrite(_txPin, HIGH);
        delay(SLOW_INIT_BIT_DURATION_MS);

        // Re-enable UART
        _serial.begin(9600, SERIAL_8N1, _rxPin, _txPin);
        delay(50);

        return true;
    }

    bool waitForKeywordBytes() {
        unsigned long start = millis();
        while (_serial.available() < 2) {
            if (millis() - start > RESPONSE_TIMEOUT_MS) return false;
            delay(1);
        }

        _keyByte1 = _serial.read();
        _keyByte2 = _serial.read();

        Serial.printf("[Yamaha] Keywords: 0x%02X 0x%02X\n", _keyByte1, _keyByte2);
        return true;
    }

    bool completeHandshake() {
        // Send inverted keyword byte 2 as acknowledgment
        uint8_t ack = ~_keyByte2;
        _serial.write(ack);
        _serial.flush();

        // Wait for ECU final ACK
        unsigned long start = millis();
        while (!_serial.available()) {
            if (millis() - start > RESPONSE_TIMEOUT_MS) return false;
            delay(1);
        }

        uint8_t ecuAck = _serial.read();
        Serial.printf("[Yamaha] ECU ACK: 0x%02X\n", ecuAck);
        return true;
    }

    // ==========================================
    // FRAME COMMUNICATION
    // ==========================================
    void sendFrame(const uint8_t* data, int len) {
        for (int i = 0; i < len; i++) {
            _serial.write(data[i]);
            delay(5);  // Inter-byte delay for Yamaha ECU
        }
        _serial.flush();
    }

    int readFrame(uint8_t* buffer, int maxLen, int timeoutMs) {
        unsigned long start = millis();
        int idx = 0;

        while (idx < maxLen) {
            if (_serial.available()) {
                buffer[idx++] = _serial.read();
                start = millis();  // Reset timeout on each byte
            } else if (millis() - start > (uint32_t)timeoutMs) {
                break;
            }
            delay(1);
        }

        return idx;
    }

    uint8_t calculateChecksum(const uint8_t* data, int len) {
        uint8_t sum = 0;
        for (int i = 0; i < len; i++) {
            sum += data[i];
        }
        return (~sum) & 0xFF;  // One's complement
    }

    // ==========================================
    // PARAMETER MAPPING
    // ==========================================
    uint8_t getParamId(const String& name) {
        if (name == "RPM")     return 0x00;
        if (name == "TPS")     return 0x01;
        if (name == "ECT")     return 0x02;
        if (name == "IAT")     return 0x03;
        if (name == "MAP")     return 0x04;
        if (name == "BATT")    return 0x05;
        if (name == "INJ_PW")  return 0x06;
        if (name == "IGN_ADV") return 0x07;
        return 0xFF;
    }

    float decodeParam(const String& name, uint8_t highByte, uint8_t lowByte) {
        if (name == "RPM") {
            // RPM = (highByte * 256 + lowByte) / 4
            return ((float)highByte * 256.0f + (float)lowByte) / 4.0f;
        }
        if (name == "TPS") {
            // TPS percentage = value * 100 / 255
            return (float)highByte * 100.0f / 255.0f;
        }
        if (name == "ECT" || name == "IAT") {
            // Temperature = value - 40 (degrees Celsius)
            return (float)highByte - 40.0f;
        }
        if (name == "MAP") {
            // MAP in kPa
            return (float)highByte;
        }
        if (name == "BATT") {
            // Battery voltage = value * 0.0733
            return (float)highByte * 0.0733f;
        }
        if (name == "INJ_PW") {
            // Injector pulse width in ms = (high*256 + low) / 1000
            return ((float)highByte * 256.0f + (float)lowByte) / 1000.0f;
        }
        if (name == "IGN_ADV") {
            // Ignition advance in degrees
            return (float)highByte - 20.0f;
        }
        return 0.0f;
    }

    // ==========================================
    // DTC DESCRIPTION TABLE
    // ==========================================
    String getDTCDescription(uint8_t code) {
        switch (code) {
            case 12: return "Crankshaft Position Sensor";
            case 14: return "Intake Air Pressure Sensor";
            case 19: return "ECT Sensor (Engine Coolant Temperature)";
            case 21: return "Intake Air Temperature Sensor";
            case 22: return "Intake Air Pressure Sensor (Low)";
            case 23: return "Intake Air Temperature Sensor (High)";
            case 33: return "Throttle Position Sensor";
            case 39: return "Oxygen Sensor (Heated)";
            case 41: return "Lean Malfunction";
            case 42: return "Rich Malfunction";
            case 44: return "PAIR Solenoid Valve";
            case 46: return "Vehicle Speed Sensor";
            case 49: return "FI System (ECU Internal)";
            case 51: return "Throttle Position Sensor (Low)";
            case 52: return "Throttle Position Sensor (High)";
            case 54: return "Ignition Coil";
            case 55: return "Lean Malfunction (Secondary)";
            case 56: return "Rich Malfunction (Secondary)";
            default: return "Unknown DTC (" + String(code) + ")";
        }
    }
};

#endif // YAMAHA_KLINE_PLUGIN_H
