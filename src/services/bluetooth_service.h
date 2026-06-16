/**
 * @file bluetooth_service.h
 * @brief Bluetooth Service for mobile app communication.
 * Supports both Classic SPP (ESP32) and BLE (ESP32-S3) via compile-time switch.
 */

#ifndef BLUETOOTH_SERVICE_H
#define BLUETOOTH_SERVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "core/protocol_plugin.h"

#ifdef USE_BLE_INSTEAD_OF_BT_CLASSIC
// ==========================================
// BLE Implementation (ESP32-S3)
// ==========================================
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "6e400001-b5a3-f393-e0a9-e50e24dcca9e"  // Nordic UART Service
#define CHAR_TX_UUID        "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  // TX (notify)
#define CHAR_RX_UUID        "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  // RX (write)

class BluetoothService : public BLECharacteristicCallbacks {
public:
    BluetoothService() : initialized(false), clientConnected(false), 
                          pServer(nullptr), pTxCharacteristic(nullptr) {}

    void init(const String& deviceName) {
        BLEDevice::init(deviceName.c_str());
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks(this));

        BLEService* pService = pServer->createService(SERVICE_UUID);

        // TX Characteristic (notify to client)
        pTxCharacteristic = pService->createCharacteristic(
            CHAR_TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
        pTxCharacteristic->addDescriptor(new BLE2902());

        // RX Characteristic (receive from client)
        BLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
            CHAR_RX_UUID, BLECharacteristic::PROPERTY_WRITE);
        pRxCharacteristic->setCallbacks(this);

        pService->start();
        pServer->getAdvertising()->start();
        initialized = true;
        Serial.println("[BLE] Started: " + deviceName);
    }

    bool isConnected() {
        return initialized && clientConnected;
    }

    void sendTelemetry(float batt, float rpm, float tps_pct, float temp,
                       float healthScore, float fuelRate) {
        if (!isConnected()) return;

        StaticJsonDocument<256> doc;
        doc["type"] = "telemetry";
        doc["batt"] = serialized(String(batt, 2));
        doc["rpm"] = serialized(String(rpm, 0));
        doc["tps"] = serialized(String(tps_pct, 1));
        doc["temp"] = serialized(String(temp, 1));
        doc["health"] = serialized(String(healthScore, 1));
        doc["fuel_rate"] = serialized(String(fuelRate, 2));

        char buf[256];
        size_t len = serializeJson(doc, buf, sizeof(buf));
        sendBLE(buf, len);
    }

    void sendDTCs(const std::vector<DTC>& dtcs) {
        if (!isConnected()) return;

        DynamicJsonDocument doc(2048);
        doc["type"] = "dtc_list";
        JsonArray arr = doc.createNestedArray("codes");
        for (const auto& dtc : dtcs) {
            JsonObject obj = arr.createNestedObject();
            obj["code"] = dtc.code;
            obj["desc"] = dtc.description;
            obj["active"] = dtc.is_active;
        }

        String output;
        serializeJson(doc, output);
        sendBLE(output.c_str(), output.length());
    }

    void sendAlert(const String& message, const String& level) {
        if (!isConnected()) return;

        StaticJsonDocument<128> doc;
        doc["type"] = "alert";
        doc["msg"] = message;
        doc["level"] = level;

        char buf[128];
        size_t len = serializeJson(doc, buf, sizeof(buf));
        sendBLE(buf, len);
    }

    String checkIncoming() {
        if (!isConnected()) return "";
        if (rxBuffer.isEmpty()) return "";

        String cmd = rxBuffer;
        rxBuffer = "";
        cmd.trim();
        if (!cmd.isEmpty()) {
            Serial.println("[BLE] Received: " + cmd);
        }
        return cmd;
    }

    // BLECharacteristicCallbacks
    void onWrite(BLECharacteristic* pCharacteristic) override {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            rxBuffer = String(value.c_str());
        }
    }

private:
    bool initialized;
    bool clientConnected;
    BLEServer* pServer;
    BLECharacteristic* pTxCharacteristic;
    String rxBuffer;

    void sendBLE(const char* data, size_t len) {
        if (!pTxCharacteristic) return;
        // BLE MTU is typically 20 bytes, send in chunks
        size_t offset = 0;
        while (offset < len) {
            size_t chunkSize = min((size_t)20, len - offset);
            pTxCharacteristic->setValue((uint8_t*)(data + offset), chunkSize);
            pTxCharacteristic->notify();
            offset += chunkSize;
            delay(10); // Give BLE stack time between chunks
        }
    }

    class ServerCallbacks : public BLEServerCallbacks {
    public:
        ServerCallbacks(BluetoothService* svc) : service(svc) {}
        void onConnect(BLEServer* pServer) override { service->clientConnected = true; }
        void onDisconnect(BLEServer* pServer) override {
            service->clientConnected = false;
            pServer->getAdvertising()->start(); // Restart advertising
        }
    private:
        BluetoothService* service;
    };
};

#else
// ==========================================
// Classic Bluetooth SPP (ESP32 original)
// ==========================================
#include <BluetoothSerial.h>

class BluetoothService {
public:
    BluetoothService() : initialized(false) {}

    void init(const String& deviceName) {
        if (!SerialBT.begin(deviceName)) {
            Serial.println("[BT] Failed to initialize Bluetooth");
            return;
        }
        initialized = true;
        Serial.println("[BT] Bluetooth started: " + deviceName);
    }

    bool isConnected() {
        return initialized && SerialBT.hasClient();
    }

    void sendTelemetry(float batt, float rpm, float tps_pct, float temp,
                       float healthScore, float fuelRate) {
        if (!isConnected()) return;

        StaticJsonDocument<256> doc;
        doc["type"] = "telemetry";
        doc["batt"] = serialized(String(batt, 2));
        doc["rpm"] = serialized(String(rpm, 0));
        doc["tps"] = serialized(String(tps_pct, 1));
        doc["temp"] = serialized(String(temp, 1));
        doc["health"] = serialized(String(healthScore, 1));
        doc["fuel_rate"] = serialized(String(fuelRate, 2));

        String output;
        serializeJson(doc, output);
        SerialBT.println(output);
    }

    void sendDTCs(const std::vector<DTC>& dtcs) {
        if (!isConnected()) return;

        DynamicJsonDocument doc(2048);
        doc["type"] = "dtc_list";
        JsonArray arr = doc.createNestedArray("codes");
        for (const auto& dtc : dtcs) {
            JsonObject obj = arr.createNestedObject();
            obj["code"] = dtc.code;
            obj["desc"] = dtc.description;
            obj["active"] = dtc.is_active;
        }

        String output;
        serializeJson(doc, output);
        SerialBT.println(output);
    }

    void sendAlert(const String& message, const String& level) {
        if (!isConnected()) return;

        StaticJsonDocument<128> doc;
        doc["type"] = "alert";
        doc["msg"] = message;
        doc["level"] = level;

        String output;
        serializeJson(doc, output);
        SerialBT.println(output);
    }

    String checkIncoming() {
        if (!isConnected()) return "";
        if (!SerialBT.available()) return "";

        String cmd = "";
        int maxChars = 64;
        while (SerialBT.available() && maxChars > 0) {
            char c = SerialBT.read();
            if (c == '\n' || c == '\r') break;
            cmd += c;
            maxChars--;
        }

        if (maxChars == 0) {
            while (SerialBT.available()) {
                char c = SerialBT.read();
                if (c == '\n' || c == '\r') break;
            }
        }

        cmd.trim();
        if (!cmd.isEmpty()) {
            Serial.println("[BT] Received: " + cmd);
        }
        return cmd;
    }

private:
    BluetoothSerial SerialBT;
    bool initialized;
};

#endif // USE_BLE_INSTEAD_OF_BT_CLASSIC

#endif // BLUETOOTH_SERVICE_H
