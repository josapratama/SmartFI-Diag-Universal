/**
 * @file ui_manager.h
 * @brief UI Manager for Smart FI Diagnostic Tool.
 */

#ifndef APP_UI_MANAGER_H
#define APP_UI_MANAGER_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config_types.h"

class UIManager {
public:
    UIManager() : display(nullptr) {}

    void init(const HardwareConfig& config) {
        Wire.begin(config.i2c_display.sda, config.i2c_display.scl);
        display = new Adafruit_SSD1306(128, 64, &Wire, -1);
        if (display->begin(SSD1306_SWITCHCAPVCC, config.i2c_display.address)) {
            display->clearDisplay();
            display->setTextSize(1);
            display->setTextColor(WHITE);
            display->setCursor(0, 0);
            display->println("Smart FI Diag V1");
            display->display();
        }
    }

    void update(float batteryV, float tpsV, float rpm, float temp) {
        if (!display) return;
        display->clearDisplay();
        display->setCursor(0, 0);
        display->printf("Battery: %.2fV\n", batteryV);
        display->printf("TPS: %.2fV\n", tpsV);
        display->printf("RPM: %.0f\n", rpm);
        display->printf("Temp: %.1f C\n", temp);
        display->display();
    }

private:
    Adafruit_SSD1306* display;
};

#endif // APP_UI_MANAGER_H
