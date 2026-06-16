/**
 * @file suzuki_sds_plugin.h
 * @brief Suzuki Diagnostic System (SDS) Protocol Plugin.
 * 
 * SDS uses KWP2000 (ISO 14230) over K-Line, very similar to Kawasaki KDS:
 * - Baud rate: 10400 bps
 * - Fast init
 * - ECU address: 0x12
 * - Tester address: 0xF1
 * - Start diagnostic session: 0x10 0x80
 * - Request live data: 0x21 + PID
 * - Read DTCs: 0x13
 * 
 * Reference: sv650sds project, KWP2000 Arduino library by aster94
 */

#ifndef SUZUKI_SDS_PLUGIN_H
#define SUZUKI_SDS_PLUGIN_H

#include "core/protocol_plugin.h"
#include <HardwareSerial.h>

class SuzukiSDSPlugin : public ProtocolPlugin {
public:
    SuzukiSDSPlugin(int rxPin, int txPin, HardwareSerial& serial = Serial2)
        : _rxPin(rxPin), _txPin(txPin), _serial(serial), _connected(false) {}

    bool connect() override {
        Serial.println("[SDS] Initializing Suzuki Diagnostic System...");
        
        _serial.begin(10400, SERIAL_8N1, _rxPin, _txPin);
        delay(300);

        // SDS Fast Init
        if (!fastInit()) {
            Serial.println("[SDS] Fast init FAILED");
            return false;
        }

        // Wait for keyword response
        if (!waitForKeywords()) {
            Serial.println("[SDS] No keyword response");
            return false;
        }

        // Start diagnostic session
        if (!startDiagSession()) {
            Serial.println("[SDS] Start diagnostic session FAILED");
            return false;
        }

        _connected = true;
        Serial.println("[SDS] ECU Connected!");
        return true;
    }

    void disconnect() override {
        if (_connected) {
            uint8_t stopMsg[] = {0x81, ECU_ADDR, TESTER_ADDR, 0x82};
            sendWithChecksum(stopMsg, 4);
        }
        _connected = false;
        _serial.end();
        Serial.println("[SDS] Disconnected");
    }

    bool isConnected() override { return _connected; }

    std::vector<DTC> readDTCs() override {
        std::vector<DTC> dtcs;
        if (!_connected) return dtcs;

        // KWP2000 service 0x13: Read DTCs
        uint8_t request[] = {0x80, ECU_ADDR, TESTER_ADDR, 0x02, 0x13, 0xFF};
        sendWithChecksum(request, 6);

        uint8_t response[64];
        int len = readResponse(response, sizeof(response), 1000);

        if (len > 4 && response[3] == 0x53) {
            int dataStart = 5;
            for (int i = dataStart; i < len - 1; i += 2) {
                if (response[i] != 0x00) {
                    DTC dtc;
                    dtc.code = "C" + String(response[i]);
                    dtc.description = getDTCDescription(response[i]);
                    dtc.is_active = (response[i+1] & 0x01) != 0;
                    dtcs.push_back(dtc);
                }
            }
        }

        Serial.printf("[SDS] Found %d DTC(s)\n", dtcs.size());
        return dtcs;
    }

    bool clearDTCs() override {
        if (!_connected) return false;

        uint8_t request[] = {0x80, ECU_ADDR, TESTER_ADDR, 0x02, 0x14, 0xFF};
        sendWithChecksum(request, 6);

        uint8_t response[16];
        int len = readResponse(response, sizeof(response), 2000);
        
        bool success = (len > 3 && response[3] == 0x54);
        Serial.printf("[SDS] Clear DTCs: %s\n", success ? "OK" : "FAILED");
        return success;
    }

    float getLiveParam(const String& param_name) override {
        if (!_connected) return 0.0f;

        uint8_t pid = getParamPID(param_name);
        if (pid == 0xFF) return 0.0f;

        uint8_t request[] = {0x80, ECU_ADDR, TESTER_ADDR, 0x02, 0x21, pid};
        sendWithChecksum(request, 6);

        uint8_t response[32];
        int len = readResponse(response, sizeof(response), 500);

        if (len > 5 && response[3] == 0x61) {
            return decodeParam(param_name, response + 5, len - 6);
        }

        return 0.0f;
    }

    String getProtocolName() override { return "Suzuki SDS (KWP2000)"; }

    bool supportsFeature(const String& feature) override {
        return (feature == "read_dtc" || feature == "clear_dtc" || 
                feature == "live_data");
    }

private:
    static const uint8_t ECU_ADDR = 0x12;
    static const uint8_t TESTER_ADDR = 0xF1;
    
    int _rxPin, _txPin;
    HardwareSerial& _serial;
    bool _connected;

    bool fastInit() {
        _serial.end();
        pinMode(_txPin, OUTPUT);
        
        // Fast init: LOW 25ms, HIGH 25ms
        digitalWrite(_txPin, LOW);
        delay(25);
        digitalWrite(_txPin, HIGH);
        delay(25);
        
        _serial.begin(10400, SERIAL_8N1, _rxPin, _txPin);
        delay(50);

        // Send wake-up
        uint8_t wakeup = 0x80;
        _serial.write(wakeup);
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
        uint8_t kw1 = _serial.read();
        uint8_t kw2 = _serial.read();
        uint8_t kw3 = _serial.read();
        Serial.printf("[SDS] Keywords: 0x%02X 0x%02X 0x%02X\n", kw1, kw2, kw3);
        return (kw1 == 0xC1);
    }

    bool startDiagSession() {
        uint8_t request[] = {0x80, ECU_ADDR, TESTER_ADDR, 0x02, 0x10, 0x80};
        sendWithChecksum(request, 6);

        uint8_t response[16];
        int len = readResponse(response, sizeof(response), 1000);
        return (len > 3 && response[3] == 0x50);
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
        // Suzuki SDS PIDs (may vary by model)
        if (name == "RPM")     return 0x09;
        if (name == "TPS")     return 0x04;
        if (name == "ECT")     return 0x06;
        if (name == "IAT")     return 0x07;
        if (name == "BATT")    return 0x0C;
        if (name == "SPEED")   return 0x0A;
        if (name == "MAP")     return 0x05;
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
        if (name == "MAP") return (float)data[0];
        return 0.0f;
    }

    String getDTCDescription(uint8_t code) {
        // Basic lookup - full table in suzuki_dtc.json
        switch (code) {
            case 12: return "Crankshaft Position Sensor";
            case 13: return "Intake Air Pressure Sensor";
            case 14: return "Throttle Position Sensor";
            case 15: return "Engine Coolant Temp Sensor";
            case 25: return "Fuel Injector";
            case 31: return "Ignition Coil";
            case 44: return "ECU Internal Error";
            default: return "Suzuki DTC C" + String(code);
        }
    }
};

#endif // SUZUKI_SDS_PLUGIN_H
