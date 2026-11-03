/**
 * @file honda_kline_plugin.h
 * @brief Implementation of Honda PGM-FI K-Line Protocol (ISO 14230 / KWP2000 Fast Init).
 */

#ifndef HONDA_KLINE_PLUGIN_H
#define HONDA_KLINE_PLUGIN_H

#include "protocol_plugin.h"
#include <HardwareSerial.h>

class HondaKLinePlugin : public ProtocolPlugin {
public:
    HondaKLinePlugin(int rx, int tx, HardwareSerial* serialPort = &Serial2) 
        : rx_pin(rx), tx_pin(tx), kSerial(serialPort), connected(false), lastPollTime(0) {
        cachedECT = 0.0f;
        cachedMAP = 0.0f;
        cachedTPS = 0.0f;
        cachedRPM = 0.0f;
        cachedBatt = 0.0f;
    }

    ~HondaKLinePlugin() override {
        disconnect();
    }

    bool connect() override {
        Serial.println("[Honda K-Line] Initializing Honda PGM-FI Protocol...");
        disconnect();

        // 1. Fast Init Sequence (ISO 14230-2)
        // Set TX pin as output for fast-init pulse
        pinMode(tx_pin, OUTPUT);
        digitalWrite(tx_pin, HIGH);
        delay(300); // Bus idle time (Tidle >= 300ms)

        // Fast-init Low pulse: 25ms +/- 1ms
        digitalWrite(tx_pin, LOW);
        delay(25);

        // Fast-init High pulse: 25ms +/- 1ms
        digitalWrite(tx_pin, HIGH);
        delay(25);

        // 2. Initialize Hardware Serial at 10,400 baud, 8N1
        kSerial->begin(10400, SERIAL_8N1, rx_pin, tx_pin);
        kSerial->setTimeout(350);

        // Clear RX buffer (flush echo/line noise)
        flushRx();

        // 3. Send Honda Start Communication Request
        // Command: 0x72 (Destination), 0x05 (Length), 0x00 (Source/Command), 0xF0 (Mode)
        // Checksum: (0x100 - (sum & 0xFF)) = 0x99
        const uint8_t initRequest[] = { 0x72, 0x05, 0x00, 0xF0, 0x99 };
        
        for (int attempt = 0; attempt < 3; attempt++) {
            if (attempt > 0) {
                delay(100);
                flushRx();
            }

            if (sendPacket(initRequest, sizeof(initRequest))) {
                uint8_t resp[32];
                int len = readPacket(resp, sizeof(resp), 400);
                if (len >= 3) {
                    // Valid ECU response received
                    Serial.printf("[Honda K-Line] ECU Connected! Resp len: %d, header: 0x%02X\n", len, resp[0]);
                    connected = true;
                    lastPollTime = millis();
                    return true;
                }
            }
        }

        Serial.println("[Honda K-Line] Connection failed (No response from ECU).");
        connected = false;
        return false;
    }

    void disconnect() override {
        if (connected) {
            // Optional Stop Communication frame
            uint8_t stopCmd[] = { 0x72, 0x04, 0x82, 0x00 };
            stopCmd[3] = calcChecksum(stopCmd, 3);
            sendPacket(stopCmd, 4);
            delay(50);
        }
        connected = false;
        kSerial->end();
        pinMode(tx_pin, INPUT_PULLUP);
    }

    bool isConnected() override {
        return connected;
    }

    std::vector<DTC> readDTCs() override {
        std::vector<DTC> dtcs;
        if (!connected) return dtcs;

        // Honda DTC Query Command: { 0x72, 0x05, 0x71, 0x00, checksum }
        uint8_t dtcCmd[5] = { 0x72, 0x05, 0x71, 0x00, 0x00 };
        dtcCmd[4] = calcChecksum(dtcCmd, 4);

        if (sendPacket(dtcCmd, sizeof(dtcCmd))) {
            uint8_t resp[64];
            int len = readPacket(resp, sizeof(resp), 400);
            if (len >= 5 && resp[2] == 0x71) {
                int count = (len - 4); // Number of DTC fault bytes
                for (int i = 0; i < count; i++) {
                    uint8_t codeVal = resp[3 + i];
                    if (codeVal > 0) {
                        DTC dtc;
                        dtc.code = String(codeVal);
                        dtc.description = getHondaDTCName(codeVal);
                        dtc.is_active = true;
                        dtcs.push_back(dtc);
                    }
                }
            }
        }
        return dtcs;
    }

    bool clearDTCs() override {
        if (!connected) return false;

        // Honda Clear DTCs: { 0x72, 0x04, 0x70, checksum }
        uint8_t clearCmd[4] = { 0x72, 0x04, 0x70, 0x00 };
        clearCmd[3] = calcChecksum(clearCmd, 3);

        if (sendPacket(clearCmd, sizeof(clearCmd))) {
            uint8_t resp[16];
            int len = readPacket(resp, sizeof(resp), 400);
            return (len >= 3);
        }
        return false;
    }

    float getLiveParam(const String& param_name) override {
        if (!connected) return 0.0f;

        // Refresh live data table every 80ms
        uint32_t now = millis();
        if (now - lastPollTime > 80) {
            pollLiveData();
            lastPollTime = now;
        }

        if (param_name == "ECT") return cachedECT;
        if (param_name == "MAP") return cachedMAP;
        if (param_name == "TPS") return cachedTPS;
        if (param_name == "RPM") return cachedRPM;
        if (param_name == "BATT") return cachedBatt;

        return 0.0f;
    }

    String getProtocolName() override { 
        return "Honda PGM-FI (K-Line FastInit 10400)"; 
    }
    
    bool supportsFeature(const String& feature) override {
        return (feature == "read_dtc" || feature == "live_data" || feature == "clear_dtc");
    }

private:
    int rx_pin, tx_pin;
    HardwareSerial* kSerial;
    bool connected;
    uint32_t lastPollTime;

    float cachedECT;
    float cachedMAP;
    float cachedTPS;
    float cachedRPM;
    float cachedBatt;

    uint8_t calcChecksum(const uint8_t* data, size_t len) {
        uint8_t sum = 0;
        for (size_t i = 0; i < len; i++) {
            sum += data[i];
        }
        return (uint8_t)(0x100 - sum);
    }

    void flushRx() {
        while (kSerial->available() > 0) {
            kSerial->read();
        }
    }

    bool sendPacket(const uint8_t* data, size_t len) {
        flushRx();
        size_t written = kSerial->write(data, len);
        kSerial->flush();

        // Since K-Line transceiver is half-duplex, discard echoed bytes
        size_t echoed = 0;
        uint32_t start = millis();
        while (echoed < written && (millis() - start < 100)) {
            if (kSerial->available()) {
                kSerial->read();
                echoed++;
            }
        }
        return (written == len);
    }

    int readPacket(uint8_t* buffer, size_t maxLen, uint32_t timeoutMs) {
        uint32_t start = millis();
        size_t index = 0;

        while ((millis() - start < timeoutMs) && (index < maxLen)) {
            if (kSerial->available()) {
                buffer[index++] = kSerial->read();
                // Check if we received full expected packet length
                if (index >= 2) {
                    uint8_t expectedLen = buffer[1]; // Typically index 1 contains length
                    if (expectedLen > 0 && expectedLen <= maxLen && index >= expectedLen) {
                        break;
                    }
                }
            }
        }

        if (index > 2) {
            // Validate checksum (sum of all bytes must equal 0 modulo 256)
            uint8_t sum = 0;
            for (size_t i = 0; i < index; i++) sum += buffer[i];
            if (sum == 0) {
                return index; // Valid packet
            }
        }
        return index;
    }

    void pollLiveData() {
        // Honda Table 0 Live Data Query:
        // Header: 0x72, Len: 0x07, Mode: 0x72, Service: 0x11, Table: 0x00, Start: 0x01, Count: 0x10
        uint8_t queryCmd[8] = { 0x72, 0x07, 0x72, 0x11, 0x00, 0x01, 0x10, 0x00 };
        queryCmd[7] = calcChecksum(queryCmd, 7);

        if (sendPacket(queryCmd, sizeof(queryCmd))) {
            uint8_t resp[48];
            int len = readPacket(resp, sizeof(resp), 200);
            if (len >= 12) {
                // Decode Honda parameters:
                // RPM: 2 bytes
                cachedRPM = (float)((resp[4] << 8) | resp[5]);
                // TPS: 0-100% or ADC
                cachedTPS = (float)resp[6] * 0.392f; // Scale 0-255 to 0-100%
                // ECT: Temp in deg C (resp - 40)
                cachedECT = (float)resp[7] - 40.0f;
                // MAP: in kPa
                cachedMAP = (float)resp[8];
                // Battery voltage (resp * 0.1V)
                cachedBatt = (float)resp[9] * 0.1f;
            }
        }
    }

    String getHondaDTCName(uint8_t code) {
        switch (code) {
            case 1:  return "MAP Sensor Circuit Malfunction";
            case 7:  return "ECT / Engine Oil Temp Sensor";
            case 8:  return "TP (Throttle Position) Sensor Circuit";
            case 9:  return "IAT (Intake Air Temp) Sensor";
            case 11: return "Vehicle Speed Sensor (VSS)";
            case 12: return "Injector Circuit Malfunction";
            case 21: return "O2 (Oxygen) Sensor Malfunction";
            case 29: return "IACV (Idle Air Control Valve)";
            case 33: return "ECM / EEPROM Memory Error";
            case 54: return "Bank Angle Sensor (BAS)";
            default: return "ECU Fault Code " + String(code);
        }
    }
};

#endif // HONDA_KLINE_PLUGIN_H
