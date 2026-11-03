/**
 * @file logging_service.h
 * @brief SD Card Logging and Telemetry Service with Non-blocking Graceful Fallback.
 */

#ifndef LOGGING_SERVICE_H
#define LOGGING_SERVICE_H

#include <FS.h>
#include <SD.h>
#include <Arduino.h>
#include <vector>

class LoggingService {
public:
    static LoggingService& getInstance() {
        static LoggingService instance;
        return instance;
    }

    bool init(int cs_pin) {
        _csPin = cs_pin;
        if (!SD.begin(cs_pin)) {
            Serial.printf("[LoggingService] SD Card initialization FAILED on CS pin %d (Logging disabled)\n", cs_pin);
            _initialized = false;
            return false;
        }
        Serial.printf("[LoggingService] SD Card initialized successfully on CS pin %d\n", cs_pin);
        _initialized = true;
        return true;
    }

    bool isInitialized() const {
        return _initialized;
    }

    void startNewSession() {
        if (!_initialized) return;

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

        _currentFileName = "/log_session_" + String(sessionNum) + ".csv";
        logFile = SD.open(_currentFileName, FILE_WRITE);
        if (logFile) {
            logFile.println("Timestamp,Battery_V,TPS_V,RPM,Temp_C,MIL_Active");
            logFile.flush();
            Serial.println("[LoggingService] Logging started: " + _currentFileName);
        } else {
            Serial.println("[LoggingService] Failed to create log file: " + _currentFileName);
        }
    }

    void logData(float battery_v, float tps_v, float rpm, float temp, bool mil_active) {
        if (!_initialized || !logFile) {
            return;
        }
        
        // BUG FIX #3: Removed !logFile.available() which checked read-stream and blocked writing
        int written = logFile.printf("%lu,%.2f,%.2f,%.0f,%.1f,%d\n", 
            millis(), battery_v, tps_v, rpm, temp, mil_active ? 1 : 0);
        
        if (written <= 0) {
            Serial.println("[LoggingService] Write failed, stopping session");
            logFile.close();
            return;
        }
        
        // Flush periodically every 2 seconds to ensure data integrity
        static uint32_t lastFlush = 0;
        if (millis() - lastFlush > 2000) {
            logFile.flush();
            lastFlush = millis();
        }
    }

    void stopSession() {
        if (logFile) {
            logFile.flush();
            logFile.close();
            Serial.println("[LoggingService] Logging session stopped.");
        }
    }

    String getCurrentLogFile() const {
        return _currentFileName;
    }

    std::vector<String> listLogFiles() {
        std::vector<String> fileList;
        if (!_initialized) return fileList;

        File root = SD.open("/");
        if (!root || !root.isDirectory()) return fileList;

        File file = root.openNextFile();
        while (file) {
            String name = file.name();
            if (name.startsWith("/") || name.startsWith("log_session_") || name.endsWith(".csv")) {
                fileList.push_back(name);
            }
            file = root.openNextFile();
        }
        return fileList;
    }

private:
    LoggingService() : _initialized(false), _csPin(-1), _currentFileName("") {}
    bool _initialized;
    int _csPin;
    String _currentFileName;
    File logFile;
};

#endif // LOGGING_SERVICE_H
