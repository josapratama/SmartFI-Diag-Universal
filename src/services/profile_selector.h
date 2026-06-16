/**
 * @file profile_selector.h
 * @brief Vehicle Profile Selector - scan SD card and let user choose via OLED.
 */

#ifndef PROFILE_SELECTOR_H
#define PROFILE_SELECTOR_H

#include <Arduino.h>
#include <SD.h>
#include <vector>
#include <Adafruit_SSD1306.h>

struct ProfileEntry {
    String filename;   // Full path on SD
    String displayName; // Extracted name for UI
};

class ProfileSelector {
public:
    ProfileSelector() : selectedIndex(0) {}

    void scanProfiles() {
        profiles.clear();
        File dir = SD.open("/configs/vehicles");
        if (!dir || !dir.isDirectory()) {
            Serial.println("[ProfileSelector] /configs/vehicles not found");
            return;
        }

        File entry;
        while ((entry = dir.openNextFile())) {
            String fullPath = entry.name();
            // ESP32 SD library returns full path, extract filename only
            String name = fullPath;
            int lastSlash = name.lastIndexOf('/');
            if (lastSlash >= 0) {
                name = name.substring(lastSlash + 1);
            }
            
            // Only include model_* files (skip base_ and family_ which are parents)
            if (name.startsWith("model_") && name.endsWith(".json")) {
                ProfileEntry pe;
                pe.filename = fullPath.startsWith("/") ? fullPath : "/configs/vehicles/" + name;
                // Extract display name from filename
                pe.displayName = name.substring(6); // Remove "model_"
                pe.displayName = pe.displayName.substring(0, pe.displayName.length() - 5); // Remove ".json"
                pe.displayName.replace("_", " ");
                profiles.push_back(pe);
            }
            entry.close();
        }
        dir.close();

        Serial.printf("[ProfileSelector] Found %d profiles\n", profiles.size());
    }

    const std::vector<ProfileEntry>& getProfiles() const { return profiles; }

    /**
     * Show selection UI on OLED. Blocks until user selects.
     * Returns the full path of selected profile.
     */
    String showSelectionUI(Adafruit_SSD1306* display, int btnUp, int btnDown, int btnSelect) {
        if (profiles.empty()) return "";

        pinMode(btnUp, INPUT_PULLUP);
        pinMode(btnDown, INPUT_PULLUP);
        pinMode(btnSelect, INPUT_PULLUP);

        selectedIndex = 0;
        bool selected = false;

        while (!selected) {
            // Draw menu
            display->clearDisplay();
            display->setTextSize(1);
            display->setTextColor(WHITE);
            display->setCursor(0, 0);
            display->println("== Select Vehicle ==");
            display->println("");

            // Show up to 4 items with scroll
            int startIdx = (selectedIndex > 2) ? selectedIndex - 2 : 0;
            int endIdx = min(startIdx + 4, (int)profiles.size());

            for (int i = startIdx; i < endIdx; i++) {
                if (i == selectedIndex) {
                    display->print("> ");
                } else {
                    display->print("  ");
                }
                // Truncate long names
                String name = profiles[i].displayName;
                if (name.length() > 18) name = name.substring(0, 18);
                display->println(name);
            }

            // Show scroll indicator
            display->setCursor(100, 56);
            display->printf("%d/%d", selectedIndex + 1, profiles.size());
            display->display();

            // Handle buttons (with debounce)
            delay(50);
            if (digitalRead(btnUp) == LOW) {
                if (selectedIndex > 0) selectedIndex--;
                delay(200); // Debounce
            }
            if (digitalRead(btnDown) == LOW) {
                if (selectedIndex < (int)profiles.size() - 1) selectedIndex++;
                delay(200);
            }
            if (digitalRead(btnSelect) == LOW) {
                selected = true;
                delay(200);
            }
        }

        // Show confirmation
        display->clearDisplay();
        display->setCursor(0, 20);
        display->println("Selected:");
        display->println(profiles[selectedIndex].displayName);
        display->display();
        delay(1000);

        return profiles[selectedIndex].filename;
    }

private:
    std::vector<ProfileEntry> profiles;
    int selectedIndex;
};

#endif // PROFILE_SELECTOR_H
