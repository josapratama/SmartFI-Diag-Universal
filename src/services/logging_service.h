/**
 * @file logging_service.h
 * @brief SD Card Logging and Telemetry Service.
 */

#ifndef LOGGING_SERVICE_H
#define LOGGING_SERVICE_H

#include <FS.h>
#include <SD.h>
#include <Arduino.h>

class LoggingService {
public:
    static LoggingService& getInstance() {
        static LoggingService instance;
        return instance;
    }

    bool init(int cs_pin) {
        if (!SD.begin(cs_pin)) {
            Serial.println("SD Card initialization FAILED");
            return false;
        }
        Serial.println("SD Card initialized");
        return true;
    }

    void startNewSession() {
        // Generate unique filename using session counter stored on SD
        int sessionNum = 0;
        File counterFile = SD.open("/session_counter.txt", FILE_READ);
        if (counterFile) {
            sessionNum = counterFile.parseInt();
            counterFile.close();
        }
        sessionNum++;
        counterFile = SD.open("/session_counter.txt", FILE_WRITE);
        if (counterFile) {
            counterFile.print(sessionNum);
            counterFile.close();
        }

        String fileName = "/log_session_" + String(sessionNum) + ".csv";
        logFile = SD.open(fileName, FILE_WRITE);
        if (logFile) {
            logFile.println("Timestamp,Battery_V,TPS_V,RPM,Temp_C,MIL_Active");
            Serial.println("Logging to: " + fileName);
        } else {
            Serial.println("Failed to create log file: " + fileName);
        }
    }

    void logData(float battery_v, float tps_v, float rpm, float temp, bool mil_active) {
        if (!logFile || !logFile.available()) {
            // File handle invalid, try to skip gracefully
            return;
        }
        
        int written = logFile.printf("%lu,%.2f,%.2f,%.0f,%.1f,%d\n", 
            millis(), battery_v, tps_v, rpm, temp, mil_active ? 1 : 0);
        
        if (written <= 0) {
            // Write failed - SD card may have been removed
            Serial.println("[Log] Write failed, stopping session");
            logFile.close();
            return;
        }
        
        // Flush periodically
        static uint32_t lastFlush = 0;
        if (millis() - lastFlush > 5000) {
            logFile.flush();
            lastFlush = millis();
        }
    }

    void stopSession() {
        if (logFile) {
            logFile.close();
        }
    }

private:
    LoggingService() {}
    File logFile;
};

#endif // LOGGING_SERVICE_H
