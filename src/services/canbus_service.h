/**
 * @file canbus_service.h
 * @brief CAN Bus Service (Optional) - for newer motorcycles with CAN protocol.
 * Gracefully disabled if MCP2515 module not connected.
 * Requires: MCP2515 CAN module connected via SPI (separate CS pin).
 */

#ifndef CANBUS_SERVICE_H
#define CANBUS_SERVICE_H

#include <Arduino.h>
#include <SPI.h>

// MCP2515 Register addresses
#define MCP_CANSTAT  0x0E
#define MCP_CANCTRL  0x0F
#define MCP_CNF3     0x28
#define MCP_CNF2     0x29
#define MCP_CNF1     0x2A
#define MCP_RXB0CTRL 0x60
#define MCP_RXB0SIDH 0x61

struct CANMessage {
    uint32_t id;
    uint8_t len;
    uint8_t data[8];
};

class CANBusService {
public:
    CANBusService() : available(false), csPin(-1) {}

    /**
     * Initialize CAN bus via MCP2515.
     * @param cs Chip Select pin for MCP2515 (-1 to disable)
     * @param speed CAN bus speed (500000 for 500kbps typical motorcycle CAN)
     */
    void init(int cs, uint32_t speed = 500000) {
        if (cs < 0) {
            Serial.println("[CAN] Disabled (no CS pin configured)");
            return;
        }

        csPin = cs;
        pinMode(csPin, OUTPUT);
        digitalWrite(csPin, HIGH);

        // Try to communicate with MCP2515
        if (!detectMCP2515()) {
            Serial.println("[CAN] MCP2515 not detected - CAN disabled");
            csPin = -1;
            return;
        }

        // Reset MCP2515
        resetMCP2515();
        
        // Configure bit timing for specified speed
        configureBitTiming(speed);
        
        // Set normal mode
        setMode(0x00); // Normal mode
        
        available = true;
        Serial.printf("[CAN] Initialized at %d bps\n", speed);
    }

    bool isAvailable() const { return available; }

    void update() {
        if (!available) return;
        // Check for received messages
        if (hasMessage()) {
            CANMessage msg = readMessage();
            processMessage(msg);
        }
    }

    // Get last received RPM from CAN (PID 0x0C)
    float getCANRpm() const { return canRpm; }
    float getCANSpeed() const { return canSpeed; }
    float getCANThrottle() const { return canThrottle; }
    float getCANCoolantTemp() const { return canCoolantTemp; }

    bool hasMessage() {
        if (!available) return false;
        uint8_t status = readRegister(0x2C); // Read RX status
        return (status & 0x40) != 0; // RXB0 has message
    }

    CANMessage readMessage() {
        CANMessage msg = {0, 0, {0}};
        if (!available) return msg;

        // Read ID
        uint8_t sidh = readRegister(MCP_RXB0SIDH);
        uint8_t sidl = readRegister(MCP_RXB0SIDH + 1);
        msg.id = ((uint32_t)sidh << 3) | (sidl >> 5);

        // Read length
        msg.len = readRegister(MCP_RXB0SIDH + 4) & 0x0F;
        if (msg.len > 8) msg.len = 8;

        // Read data
        for (int i = 0; i < msg.len; i++) {
            msg.data[i] = readRegister(MCP_RXB0SIDH + 5 + i);
        }

        // Clear RX flag
        modifyRegister(0x2C, 0x01, 0x00);

        return msg;
    }

    // Send OBD2 request (for CAN-based diagnostics)
    bool sendOBD2Request(uint8_t pid) {
        if (!available) return false;
        CANMessage msg;
        msg.id = 0x7DF; // OBD2 broadcast request
        msg.len = 8;
        msg.data[0] = 0x02; // Number of additional bytes
        msg.data[1] = 0x01; // Mode 01 (current data)
        msg.data[2] = pid;
        msg.data[3] = 0x00;
        msg.data[4] = 0x00;
        msg.data[5] = 0x00;
        msg.data[6] = 0x00;
        msg.data[7] = 0x00;
        return sendMessage(msg);
    }

private:
    bool available;
    int csPin;
    float canRpm = 0;
    float canSpeed = 0;
    float canThrottle = 0;
    float canCoolantTemp = 0;

    void processMessage(const CANMessage& msg) {
        // Process OBD2 responses (ID 0x7E8)
        if (msg.id == 0x7E8 && msg.len >= 4) {
            uint8_t pid = msg.data[2];
            switch (pid) {
                case 0x0C: // RPM
                    canRpm = ((msg.data[3] * 256.0f) + msg.data[4]) / 4.0f;
                    break;
                case 0x0D: // Vehicle speed
                    canSpeed = msg.data[3];
                    break;
                case 0x11: // Throttle position
                    canThrottle = msg.data[3] * 100.0f / 255.0f;
                    break;
                case 0x05: // Coolant temp
                    canCoolantTemp = msg.data[3] - 40.0f;
                    break;
            }
        }
    }

    bool detectMCP2515() {
        // Read CANSTAT register - should return a valid mode
        uint8_t val = readRegister(MCP_CANSTAT);
        return (val != 0xFF && val != 0x00); // 0xFF/0x00 = no device
    }

    void resetMCP2515() {
        digitalWrite(csPin, LOW);
        SPI.transfer(0xC0); // Reset command
        digitalWrite(csPin, HIGH);
        delay(10);
    }

    void configureBitTiming(uint32_t speed) {
        // Set configuration mode
        setMode(0x80);
        
        // Bit timing for 8MHz crystal, 500kbps
        if (speed == 500000) {
            writeRegister(MCP_CNF1, 0x00);
            writeRegister(MCP_CNF2, 0x90);
            writeRegister(MCP_CNF3, 0x02);
        } else if (speed == 250000) {
            writeRegister(MCP_CNF1, 0x01);
            writeRegister(MCP_CNF2, 0x90);
            writeRegister(MCP_CNF3, 0x02);
        }

        // Accept all messages
        writeRegister(0x20, 0x00); // RXM0SIDH = 0 (mask)
        writeRegister(0x21, 0x00);
        writeRegister(0x24, 0x00);
        writeRegister(0x25, 0x00);
    }

    void setMode(uint8_t mode) {
        modifyRegister(MCP_CANCTRL, 0xE0, mode);
        delay(10);
    }

    bool sendMessage(const CANMessage& msg) {
        // Load TX buffer 0
        writeRegister(0x31, (msg.id >> 3) & 0xFF);  // TXB0SIDH
        writeRegister(0x32, (msg.id << 5) & 0xE0);  // TXB0SIDL
        writeRegister(0x35, msg.len);                 // TXB0DLC
        for (int i = 0; i < msg.len; i++) {
            writeRegister(0x36 + i, msg.data[i]);
        }
        // Request to send
        modifyRegister(0x30, 0x08, 0x08); // TXB0CTRL.TXREQ = 1
        return true;
    }

    uint8_t readRegister(uint8_t addr) {
        digitalWrite(csPin, LOW);
        SPI.transfer(0x03); // Read command
        SPI.transfer(addr);
        uint8_t val = SPI.transfer(0x00);
        digitalWrite(csPin, HIGH);
        return val;
    }

    void writeRegister(uint8_t addr, uint8_t val) {
        digitalWrite(csPin, LOW);
        SPI.transfer(0x02); // Write command
        SPI.transfer(addr);
        SPI.transfer(val);
        digitalWrite(csPin, HIGH);
    }

    void modifyRegister(uint8_t addr, uint8_t mask, uint8_t val) {
        digitalWrite(csPin, LOW);
        SPI.transfer(0x05); // Bit modify command
        SPI.transfer(addr);
        SPI.transfer(mask);
        SPI.transfer(val);
        digitalWrite(csPin, HIGH);
    }
};

#endif // CANBUS_SERVICE_H
