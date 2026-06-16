/**
 * @file gps_service.h
 * @brief GPS Service (Optional) - location logging and speed measurement.
 * Gracefully disabled if GPS module not connected.
 * Supports NEO-6M/NEO-7M/NEO-8M via UART (NMEA).
 */

#ifndef GPS_SERVICE_H
#define GPS_SERVICE_H

#include <Arduino.h>
#include <HardwareSerial.h>

struct GPSData {
    float latitude;
    float longitude;
    float speed_kmh;
    float altitude_m;
    int satellites;
    bool valid;
    uint32_t lastFixTime;
};

class GPSService {
public:
    GPSService() : available(false), gpsSerial(nullptr) {}

    /**
     * Initialize GPS on specified UART pins.
     * If no GPS module responds within timeout, marks as unavailable.
     * @param rxPin GPIO for GPS TX → ESP RX
     * @param txPin GPIO for GPS RX → ESP TX (usually unused)
     * @param uart UART number (1 or 2)
     */
    void init(int rxPin, int txPin, int uart = 1) {
        if (rxPin < 0) {
            Serial.println("[GPS] Disabled (no pin configured)");
            return;
        }

        if (uart == 1) {
            gpsSerial = &Serial1;
        } else {
            // UART2 might be used by K-Line, skip
            Serial.println("[GPS] UART2 reserved for K-Line, GPS disabled");
            return;
        }

        gpsSerial->begin(9600, SERIAL_8N1, rxPin, txPin);
        
        // Wait for GPS data (max 3 seconds)
        Serial.println("[GPS] Detecting module...");
        uint32_t start = millis();
        while (millis() - start < 3000) {
            if (gpsSerial->available()) {
                char c = gpsSerial->read();
                if (c == '$') { // NMEA sentence start
                    available = true;
                    Serial.println("[GPS] Module detected!");
                    return;
                }
            }
            delay(10);
        }

        Serial.println("[GPS] No module detected - GPS disabled");
        gpsSerial->end();
        gpsSerial = nullptr;
    }

    bool isAvailable() const { return available; }

    void update() {
        if (!available || !gpsSerial) return;

        while (gpsSerial->available()) {
            char c = gpsSerial->read();
            processChar(c);
        }
    }

    GPSData getData() const { return data; }
    float getSpeed() const { return data.valid ? data.speed_kmh : 0; }
    bool hasFix() const { return data.valid && data.satellites >= 3; }

private:
    bool available;
    HardwareSerial* gpsSerial;
    GPSData data = {0, 0, 0, 0, 0, false, 0};
    char nmeaBuf[128];
    int nmeaIdx = 0;

    void processChar(char c) {
        if (c == '$') {
            nmeaIdx = 0;
        }
        
        if (nmeaIdx < 127) {
            nmeaBuf[nmeaIdx++] = c;
        }

        if (c == '\n') {
            nmeaBuf[nmeaIdx] = '\0';
            parseNMEA(nmeaBuf);
            nmeaIdx = 0;
        }
    }

    void parseNMEA(const char* sentence) {
        // Parse $GPRMC for position and speed
        if (strncmp(sentence, "$GPRMC", 6) == 0 || strncmp(sentence, "$GNRMC", 6) == 0) {
            parseRMC(sentence);
        }
        // Parse $GPGGA for altitude and satellites
        else if (strncmp(sentence, "$GPGGA", 6) == 0 || strncmp(sentence, "$GNGGA", 6) == 0) {
            parseGGA(sentence);
        }
    }

    void parseRMC(const char* sentence) {
        // $GPRMC,time,status,lat,N/S,lon,E/W,speed_knots,course,date,...
        char* fields[15];
        int fieldCount = splitCSV((char*)sentence, fields, 15);
        
        if (fieldCount < 8) return;
        
        data.valid = (fields[2][0] == 'A'); // A=Active, V=Void
        
        if (data.valid) {
            data.latitude = parseCoord(fields[3], fields[4][0]);
            data.longitude = parseCoord(fields[5], fields[6][0]);
            data.speed_kmh = atof(fields[7]) * 1.852f; // knots to km/h
            data.lastFixTime = millis();
        }
    }

    void parseGGA(const char* sentence) {
        char* fields[15];
        int fieldCount = splitCSV((char*)sentence, fields, 15);
        
        if (fieldCount < 10) return;
        
        data.satellites = atoi(fields[7]);
        data.altitude_m = atof(fields[9]);
    }

    float parseCoord(const char* raw, char hemisphere) {
        // NMEA format: DDMM.MMMM or DDDMM.MMMM
        float val = atof(raw);
        int degrees = (int)(val / 100);
        float minutes = val - (degrees * 100);
        float decimal = degrees + (minutes / 60.0f);
        if (hemisphere == 'S' || hemisphere == 'W') decimal = -decimal;
        return decimal;
    }

    int splitCSV(char* str, char** fields, int maxFields) {
        int count = 0;
        fields[count++] = str;
        while (*str && count < maxFields) {
            if (*str == ',') {
                *str = '\0';
                fields[count++] = str + 1;
            }
            str++;
        }
        return count;
    }
};

#endif // GPS_SERVICE_H
