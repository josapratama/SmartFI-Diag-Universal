/**
 * @file kawasaki_kds_plugin.h
 * @brief Kawasaki Diagnostic System (KDS) Protocol Plugin.
 * 
 * KDS uses KWP2000 (ISO 14230) over K-Line:
 * - Baud rate: 10400 bps
 * - Fast init (not 5-baud slow init)
 * - ECU address: 0x11
 * - Tester address: 0xF1
 * - Init: send 0x80 at 10400 baud, wait for keyword response
 * - Start diagnostic session: 0x10 0x80
 * - Request live data: 0x21 + PID
 * - Read DTCs: 0x13 (readDiagnosticTroubleCodesByStatus)
 * 
 * Reference: Arduino forum "Bike interface OBD" thread, KDS2Bluetooth project
 */

#ifndef KAWASAKI_KDS_PLUGIN_H
#define KAWASAKI_KDS_PLUGIN_H

#include "core/protocol_plugin.h"
#include <HardwareSerial.h>

class KawasakiKDSPlugin : public ProtocolPlugin {
public:
    KawasakiKDSPlugin(int rxPin, int txPin, HardwareSerial& serial = Serial2)
        : _rxPin(rxPin), _txPin(txPin), _serial(serial), _connected(false) {}

    bool connect() override {
        Serial.println("[KDS] Initializing Kawasaki Diagnostic System...");
        
        _serial.begin(10400, SERIAL_8N1, _rxPin, _txPin);
        delay(300);

        // KDS Fast Init: send 0x80 byte to wake up ECU
        if (!fastInit()) {
            Serial.println("[KDS] Fast init FAILED");
            return false;
        }

        // Wait for ECU keyword response (0xC1 0xEA 0x8F expected)
        if (!waitForKeywords()) {
            Serial.println("[KDS] No keyword response");
            return false;
        }

        // Start diagnostic session (service 0x10, mode 0x80)
        if (!startDiagSession()) {
            Serial.println("[KDS] Start diagnostic session FAILED");
            return false;
        }

        _connected = true;
        Serial.println("[KDS] ECU Connected!");
        return true;
    }

    void disconnect() override {
        if (_connected) {
            // Send stop communication (0x82)
            uint8_t stopMsg[] = {0x81, ECU_ADDR, TESTER_ADDR, 0x82};
            sendWithChecksum(stopMsg, 4);
        }
        _connected = false;
        _serial.end();
        Serial.println("[KDS] Disconnected");
    }

    bool isConnected() override { return _connected; }

    std::vector<DTC> readDTCs() override {
        std::vector<DTC> dtcs;
        if (!_connected) return dtcs;

        // KWP2000 service 0x13: Read DTCs by status
        uint8_t request[] = {0x80, ECU_ADDR, TESTER_ADDR, 0x02, 0x13, 0xFF};
        sendWithChecksum(request, 6);

        uint8_t response[64];
        int len = readResponse(response, sizeof(response), 1000);

        if (len > 4 && response[3] == 0x53) { // Positive response to 0x13
            // Parse DTCs: format varies, typically pairs of bytes
            int dataStart = 4;
            int dataLen = response[3] - 1; // Subtract service ID byte
            for (int i = dataStart; i < dataStart + dataLen - 1; i += 2) {
                if (response[i] != 0x00 || response[i+1] != 0x00) {
                    DTC dtc;
                    dtc.code = String(response[i], HEX) + String(response[i+1], HEX);
                    dtc.description = getDTCDescription(response[i], response[i+1]);
                    dtc.is_active = true;
                    dtcs.push_back(dtc);
                }
            }
        }

        Serial.printf("[KDS] Found %d DTC(s)\n", dtcs.size());
        return dtcs;
    }

    bool clearDTCs() override {
        if (!_connected) return false;

        // KWP2000 service 0x14: Clear DTCs
        uint8_t request[] = {0x80, ECU_ADDR, TESTER_ADDR, 0x02, 0x14, 0xFF};
        sendWithChecksum(request, 6);

        uint8_t response[16];
        int len = readResponse(response, sizeof(response), 2000);
        
        bool success = (len > 3 && response[3] == 0x54); // Positive response
        Serial.printf("[KDS] Clear DTCs: %s\n", success ? "OK" : "FAILED");
        return success;
    }

    float getLiveParam(const String& param_name) override {
        if (!_connected) return 0.0f;

        uint8_t pid = getParamPID(param_name);
        if (pid == 0xFF) return 0.0f;

        // KWP2000 service 0x21: Read data by local identifier
        uint8_t request[] = {0x80, ECU_ADDR, TESTER_ADDR, 0x02, 0x21, pid};
        sendWithChecksum(request, 6);

        uint8_t response[32];
        int len = readResponse(response, sizeof(response), 500);

        if (len > 5 && response[3] == 0x61) { // Positive response
            return decodeParam(param_name, response + 5, len - 6);
        }

        return 0.0f;
    }

    String getProtocolName() override { return "Kawasaki KDS (KWP2000)"; }

    bool supportsFeature(const String& feature) override {
        return (feature == "read_dtc" || feature == "clear_dtc" || 
                feature == "live_data" || feature == "ecu_id");
    }

private:
    static const uint8_t ECU_ADDR = 0x11;
    static const uint8_t TESTER_ADDR = 0xF1;
    
    int _rxPin, _txPin;
    HardwareSerial& _serial;
    bool _connected;

    bool fastInit() {
        // Fast init: pull K-Line low for 25ms, then high for 25ms
        _serial.end();
        pinMode(_txPin, OUTPUT);
        
        digitalWrite(_txPin, LOW);
        delay(25);
        digitalWrite(_txPin, HIGH);
        delay(25);
        
        _serial.begin(10400, SERIAL_8N1, _rxPin, _txPin);
        delay(50);

        // Send wake-up byte 0x80
        uint8_t wakeup[] = {0x80};
        _serial.write(wakeup, 1);
        _serial.flush();
        delay(50);

        return true;
    }

    bool waitForKeywords() {
        unsigned long start = millis();
        while (_serial.available() < 3) {
            if (millis() - start > 2000) return false;
            delay(1);
        }

        uint8_t kw1 = _serial.read(); // Should be 0xC1
        uint8_t kw2 = _serial.read(); // Should be 0xEA
        uint8_t kw3 = _serial.read(); // Should be 0x8F

        Serial.printf("[KDS] Keywords: 0x%02X 0x%02X 0x%02X\n", kw1, kw2, kw3);
        return (kw1 == 0xC1); // At minimum, first keyword should be 0xC1
    }

    bool startDiagSession() {
        // Start diagnostic session: service 0x10, mode 0x80
        uint8_t request[] = {0x80, ECU_ADDR, TESTER_ADDR, 0x02, 0x10, 0x80};
        sendWithChecksum(request, 6);

        uint8_t response[16];
        int len = readResponse(response, sizeof(response), 1000);

        return (len > 3 && response[3] == 0x50); // Positive response to startDiagSession
    }

    void sendWithChecksum(const uint8_t* data, int len) {
        uint8_t checksum = 0;
        for (int i = 0; i < len; i++) {
            _serial.write(data[i]);
            checksum += data[i];
            delay(5);
        }
        _serial.write(checksum);
        _serial.flush();
    }

    int readResponse(uint8_t* buffer, int maxLen, int timeoutMs) {
        unsigned long start = millis();
        int idx = 0;
        while (idx < maxLen) {
            if (_serial.available()) {
                buffer[idx++] = _serial.read();
                start = millis();
            } else if (millis() - start > (uint32_t)timeoutMs) {
                break;
            }
            delay(1);
        }
        return idx;
    }

    uint8_t getParamPID(const String& name) {
        if (name == "RPM")     return 0x09;
        if (name == "TPS")     return 0x04;
        if (name == "ECT")     return 0x06;
        if (name == "IAT")     return 0x07;
        if (name == "BATT")    return 0x0C;
        if (name == "SPEED")   return 0x0A;
        if (name == "GEAR")    return 0x0B;
        return 0xFF;
    }

    float decodeParam(const String& name, const uint8_t* data, int len) {
        if (len < 1) return 0.0f;
        if (name == "RPM" && len >= 2) return ((float)data[0] * 256 + data[1]) / 4.0f;
        if (name == "TPS") return data[0] * 100.0f / 255.0f;
        if (name == "ECT") return (float)data[0] - 40.0f;
        if (name == "IAT") return (float)data[0] - 40.0f;
        if (name == "BATT") return data[0] * 0.08f;
        if (name == "SPEED") return (float)data[0];
        if (name == "GEAR") return (float)data[0];
        return 0.0f;
    }

    String getDTCDescription(uint8_t high, uint8_t low) {
        // Kawasaki DTC format: first byte = category, second = specific
        String code = String(high) + String(low);
        // Basic descriptions - full table in kawasaki_dtc.json
        return "Kawasaki DTC " + code;
    }
};

#endif // KAWASAKI_KDS_PLUGIN_H
