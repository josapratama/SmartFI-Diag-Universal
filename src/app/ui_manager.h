/**
 * @file ui_manager.h
 * @brief Multi-Page OLED UI Manager for Smart FI Diagnostic Tool.
 * Pages: Live Data, Health Score, DTC Codes, Cranking, System Info, Maintenance.
 */

#ifndef APP_UI_MANAGER_H
#define APP_UI_MANAGER_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config_types.h"

enum UIPage {
    PAGE_LIVE = 0,
    PAGE_HEALTH,
    PAGE_DTC,
    PAGE_CRANK,
    PAGE_MAINTENANCE,
    PAGE_SYSTEM,
    PAGE_COUNT  // Total number of pages
};

class UIManager {
public:
    UIManager() : display(nullptr), currentPage(PAGE_LIVE), 
                  btnUpPin(-1), btnDownPin(-1), btnSelectPin(-1), btnBackPin(-1),
                  lastBtnTime(0) {}

    ~UIManager() {
        if (display) { delete display; display = nullptr; }
    }

    void init(const HardwareConfig& config) {
        Wire.begin(config.i2c_display.sda, config.i2c_display.scl);
        display = new Adafruit_SSD1306(128, 64, &Wire, -1);
        if (!display->begin(SSD1306_SWITCHCAPVCC, config.i2c_display.address)) {
            Serial.println("OLED init FAILED!");
            delete display;
            display = nullptr;
            return;
        }

        // Store button pins for navigation
        btnUpPin = config.buttons.up;
        btnDownPin = config.buttons.down;
        btnSelectPin = config.buttons.select;
        btnBackPin = config.buttons.back;

        // Configure button pins
        if (btnUpPin >= 0) pinMode(btnUpPin, INPUT_PULLUP);
        if (btnDownPin >= 0) pinMode(btnDownPin, INPUT_PULLUP);
        if (btnSelectPin >= 0) pinMode(btnSelectPin, INPUT_PULLUP);
        if (btnBackPin >= 0) pinMode(btnBackPin, INPUT_PULLUP);

        // Show splash
        display->clearDisplay();
        display->setTextSize(1);
        display->setTextColor(WHITE);
        display->setCursor(20, 20);
        display->println("Smart FI Diag");
        display->setCursor(35, 35);
        display->println("v2.0");
        display->display();
    }

    void update(float battV, float tpsV, float rpm, float temp) {
        if (!display) return;

        // Handle button navigation
        handleButtons();

        // Store latest data
        lastBatt = battV;
        lastTPS = tpsV;
        lastRPM = rpm;
        lastTemp = temp;

        // Render current page
        display->clearDisplay();
        drawHeader();

        switch (currentPage) {
            case PAGE_LIVE:    drawLivePage(); break;
            case PAGE_HEALTH:  drawHealthPage(); break;
            case PAGE_DTC:     drawDTCPage(); break;
            case PAGE_CRANK:   drawCrankPage(); break;
            case PAGE_MAINTENANCE: drawMaintenancePage(); break;
            case PAGE_SYSTEM:  drawSystemPage(); break;
            default: break;
        }

        display->display();
    }

    // External data setters (called from main loop)
    void setHealthScore(float score) { healthScore = score; }
    void setDTCCount(int count) { dtcCount = count; }
    void setDTCCode(const String& code) { lastDTCCode = code; }
    void setCrankVerdict(const String& verdict) { crankVerdict = verdict; }
    void setMaintenanceAlert(bool alert) { maintenanceAlert = alert; }
    void setFuelRate(float rate) { fuelRate = rate; }

    UIPage getCurrentPage() const { return currentPage; }

private:
    Adafruit_SSD1306* display;
    UIPage currentPage;
    int btnUpPin, btnDownPin, btnSelectPin, btnBackPin;
    uint32_t lastBtnTime;

    // Cached display data
    float lastBatt = 0, lastTPS = 0, lastRPM = 0, lastTemp = 0;
    float healthScore = 100;
    int dtcCount = 0;
    String lastDTCCode = "";
    String crankVerdict = "";
    bool maintenanceAlert = false;
    float fuelRate = 0;

    void handleButtons() {
        uint32_t now = millis();
        if (now - lastBtnTime < 200) return; // Debounce

        if (btnDownPin >= 0 && digitalRead(btnDownPin) == LOW) {
            currentPage = (UIPage)((currentPage + 1) % PAGE_COUNT);
            lastBtnTime = now;
        }
        if (btnUpPin >= 0 && digitalRead(btnUpPin) == LOW) {
            currentPage = (UIPage)((currentPage - 1 + PAGE_COUNT) % PAGE_COUNT);
            lastBtnTime = now;
        }
    }

    void drawHeader() {
        // Page indicator dots at top
        for (int i = 0; i < PAGE_COUNT; i++) {
            int x = 64 - (PAGE_COUNT * 4) + (i * 8);
            if (i == currentPage) {
                display->fillCircle(x, 2, 2, WHITE);
            } else {
                display->drawCircle(x, 2, 1, WHITE);
            }
        }
        // Alert icon if maintenance overdue
        if (maintenanceAlert) {
            display->setCursor(120, 0);
            display->print("!");
        }
    }

    void drawLivePage() {
        display->setTextSize(1);
        display->setCursor(0, 8);
        display->printf("Batt: %.1fV", lastBatt);
        display->setCursor(75, 8);
        display->printf("%.0fC", lastTemp);
        
        // RPM large
        display->setTextSize(2);
        display->setCursor(0, 22);
        display->printf("%.0f", lastRPM);
        display->setTextSize(1);
        display->setCursor(75, 28);
        display->print("RPM");

        // TPS bar
        display->setCursor(0, 44);
        display->printf("TPS: %.1fV", lastTPS);
        
        // Fuel rate
        display->setCursor(0, 55);
        display->printf("Fuel: %.1f cc/m", fuelRate);
    }

    void drawHealthPage() {
        display->setTextSize(1);
        display->setCursor(0, 8);
        display->println("== HEALTH SCORE ==");
        
        display->setTextSize(3);
        display->setCursor(25, 22);
        display->printf("%.0f%%", healthScore);
        
        display->setTextSize(1);
        display->setCursor(0, 52);
        if (healthScore >= 85) display->print("Status: BAIK");
        else if (healthScore >= 60) display->print("Status: PERHATIAN");
        else display->print("Status: BURUK!");
    }

    void drawDTCPage() {
        display->setTextSize(1);
        display->setCursor(0, 8);
        display->println("== KODE ERROR ==");
        
        display->setCursor(0, 22);
        if (dtcCount == 0) {
            display->setTextSize(1);
            display->println("Tidak ada error");
            display->println("Motor OK!");
        } else {
            display->printf("DTC: %d kode\n", dtcCount);
            display->println("");
            display->print("Last: ");
            display->println(lastDTCCode);
        }
    }

    void drawCrankPage() {
        display->setTextSize(1);
        display->setCursor(0, 8);
        display->println("== STARTER ==");
        
        display->setCursor(0, 22);
        display->printf("Batt: %.2fV\n", lastBatt);
        display->println("");
        
        if (crankVerdict.length() > 0) {
            // Word wrap for verdict
            String v = crankVerdict;
            while (v.length() > 21) {
                display->println(v.substring(0, 21));
                v = v.substring(21);
            }
            display->println(v);
        } else {
            display->println("Belum ada data");
            display->println("starter.");
        }
    }

    void drawMaintenancePage() {
        display->setTextSize(1);
        display->setCursor(0, 8);
        display->println("== MAINTENANCE ==");
        
        display->setCursor(0, 20);
        // This will be populated from MaintenanceService
        if (maintenanceAlert) {
            display->println("! Ada service jatuh");
            display->println("  tempo. Cek app.");
        } else {
            display->println("Semua OK");
            display->println("Belum ada service");
            display->println("yang jatuh tempo.");
        }
    }

    void drawSystemPage() {
        display->setTextSize(1);
        display->setCursor(0, 8);
        display->println("== SYSTEM INFO ==");
        
        display->setCursor(0, 20);
        display->printf("Heap: %d B\n", ESP.getFreeHeap());
        display->printf("Up: %d s\n", millis() / 1000);
        display->printf("WiFi: 192.168.4.1\n");
        display->printf("Page: %d/%d", currentPage + 1, PAGE_COUNT);
    }
};

#endif // APP_UI_MANAGER_H
