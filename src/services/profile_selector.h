/**
 * @file profile_selector.h
 * @brief Vehicle Profile Selector - scans SD card and SPIFFS with OLED choice.
 */

#ifndef PROFILE_SELECTOR_H
#define PROFILE_SELECTOR_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include <vector>
#include <set>
#include <Adafruit_SSD1306.h>
#include "core/fs_manager.h"

struct ProfileEntry {
    String filename;    // Full path
    String displayName; // Extracted name for UI
};

class ProfileSelector {
public:
    ProfileSelector() : selectedIndex(0) {}

    void scanProfiles() {
        profiles.clear();
        std::set<String> seenNames;

        auto scanDir = [&](fs::FS& fs, const char* dirPath) {
            File dir = fs.open(dirPath);
            if (!dir || !dir.isDirectory()) return;

            File entry;
            while ((entry = dir.openNextFile())) {
                String fullPath = entry.name();
                String name = fullPath;
                int lastSlash = name.lastIndexOf('/');
                if (lastSlash >= 0) {
                    name = name.substring(lastSlash + 1);
                }
                
                // Only include model_* files (skip base_ and family_ which are parents)
                if (name.startsWith("model_") && name.endsWith(".json")) {
                    if (seenNames.find(name) == seenNames.end()) {
                        seenNames.insert(name);
                        ProfileEntry pe;
                        pe.filename = fullPath.startsWith("/") ? fullPath : String(dirPath) + "/" + name;
                        pe.displayName = name.substring(6); // Remove "model_"
                        pe.displayName = pe.displayName.substring(0, pe.displayName.length() - 5); // Remove ".json"
                        pe.displayName.replace("_", " ");
                        profiles.push_back(pe);
                    }
                }
                entry.close();
            }
            dir.close();
        };

        // Scan SD first if available, then SPIFFS
        if (FSManager::getInstance().isSDAvailable()) {
            scanDir(SD, "/configs/vehicles");
        }
        if (FSManager::getInstance().isSPIFFSAvailable()) {
            scanDir(SPIFFS, "/configs/vehicles");
        }

        Serial.printf("[ProfileSelector] Found %d vehicle profiles\n", profiles.size());
    }

    const std::vector<ProfileEntry>& getProfiles() const { return profiles; }

    String showSelectionUI(Adafruit_SSD1306* display, int btnUp, int btnDown, int btnSelect) {
        if (profiles.empty()) return "";

        pinMode(btnUp, INPUT_PULLUP);
        pinMode(btnDown, INPUT_PULLUP);
        pinMode(btnSelect, INPUT_PULLUP);

        selectedIndex = 0;
        bool selected = false;

        while (!selected) {
            display->clearDisplay();
            display->setTextSize(1);
            display->setTextColor(WHITE);
            display->setCursor(0, 0);
            display->println("== Select Vehicle ==");

            int startIdx = (selectedIndex >= 4) ? selectedIndex - 3 : 0;
            for (int i = 0; i < 4 && (startIdx + i) < (int)profiles.size(); i++) {
                int idx = startIdx + i;
                display->setCursor(0, 14 + (i * 12));
                if (idx == selectedIndex) {
                    display->print("> ");
                } else {
                    display->print("  ");
                }
                display->println(profiles[idx].displayName);
            }
            display->display();

            if (digitalRead(btnUp) == LOW) {
                if (selectedIndex > 0) selectedIndex--;
                delay(180);
            }
            if (digitalRead(btnDown) == LOW) {
                if (selectedIndex < (int)profiles.size() - 1) selectedIndex++;
                delay(180);
            }
            if (digitalRead(btnSelect) == LOW) {
                selected = true;
                delay(200);
            }
            delay(10);
        }

        return profiles[selectedIndex].filename;
    }

private:
    std::vector<ProfileEntry> profiles;
    int selectedIndex;
};

#endif // PROFILE_SELECTOR_H
