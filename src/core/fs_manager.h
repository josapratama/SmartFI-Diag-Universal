/**
 * @file fs_manager.h
 * @brief Unified Filesystem Manager (Dual Storage: SD Card + Internal SPIFFS).
 */

#ifndef FS_MANAGER_H
#define FS_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <SD.h>

class FSManager {
public:
    static FSManager& getInstance() {
        static FSManager instance;
        return instance;
    }

    void init(bool sdSuccess = false) {
        _sdAvailable = sdSuccess;
        if (!_spiffsAvailable) {
            _spiffsAvailable = SPIFFS.begin(true);
            if (_spiffsAvailable) {
                Serial.println("[FSManager] Internal SPIFFS mounted successfully.");
            } else {
                Serial.println("[FSManager] SPIFFS mount FAILED.");
            }
        }
    }

    void setSDAvailable(bool avail) {
        _sdAvailable = avail;
    }

    bool isSDAvailable() const { return _sdAvailable; }
    bool isSPIFFSAvailable() const { return _spiffsAvailable; }

    /**
     * Opens a file checking SD first, then fallback to SPIFFS.
     */
    File open(const String& path, const char* mode = "r") {
        if (_sdAvailable && SD.exists(path)) {
            File f = SD.open(path, mode);
            if (f) return f;
        }
        if (_spiffsAvailable && SPIFFS.exists(path)) {
            File f = SPIFFS.open(path, mode);
            if (f) return f;
        }
        return File();
    }

    bool exists(const String& path) {
        if (_sdAvailable && SD.exists(path)) return true;
        if (_spiffsAvailable && SPIFFS.exists(path)) return true;
        return false;
    }

    fs::FS& getWebFS() {
        if (_spiffsAvailable && SPIFFS.exists("/index.html")) {
            return SPIFFS;
        }
        if (_sdAvailable && SD.exists("/index.html")) {
            return SD;
        }
        return _spiffsAvailable ? (fs::FS&)SPIFFS : (fs::FS&)SD;
    }

private:
    FSManager() : _sdAvailable(false), _spiffsAvailable(false) {}
    bool _sdAvailable;
    bool _spiffsAvailable;
};

#endif // FS_MANAGER_H

