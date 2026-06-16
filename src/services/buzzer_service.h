/**
 * @file buzzer_service.h
 * @brief Buzzer Alert Service - audio notifications for critical events.
 */

#ifndef BUZZER_SERVICE_H
#define BUZZER_SERVICE_H

#include <Arduino.h>
#include "config_types.h"

enum BuzzerPattern {
    BUZZ_NONE = 0,
    BUZZ_SINGLE_BEEP,      // 1 short beep (info)
    BUZZ_DOUBLE_BEEP,      // 2 short beeps (warning)
    BUZZ_TRIPLE_BEEP,      // 3 short beeps (DTC detected)
    BUZZ_LONG_BEEP,        // 1 long beep (critical)
    BUZZ_SOS,              // SOS pattern (overheat emergency)
    BUZZ_STARTUP_OK        // Rising tone (system ready)
};

class BuzzerService {
public:
    BuzzerService() : buzzerPin(-1), currentPattern(BUZZ_NONE), 
                      patternStep(0), lastStepTime(0), isActive(false) {}

    void init(int pin) {
        buzzerPin = pin;
        if (buzzerPin >= 0) {
            pinMode(buzzerPin, OUTPUT);
            digitalWrite(buzzerPin, LOW);
            Serial.printf("[Buzzer] Initialized on GPIO %d\n", buzzerPin);
        }
    }

    void play(BuzzerPattern pattern) {
        if (buzzerPin < 0) return;
        if (currentPattern != BUZZ_NONE && pattern == BUZZ_SINGLE_BEEP) return; // Don't interrupt
        currentPattern = pattern;
        patternStep = 0;
        lastStepTime = millis();
        isActive = true;
    }

    // Call in loop (non-blocking)
    void update() {
        if (!isActive || buzzerPin < 0) return;

        uint32_t now = millis();
        uint32_t elapsed = now - lastStepTime;

        switch (currentPattern) {
            case BUZZ_SINGLE_BEEP:
                singleBeep(elapsed);
                break;
            case BUZZ_DOUBLE_BEEP:
                doubleBeep(elapsed);
                break;
            case BUZZ_TRIPLE_BEEP:
                tripleBeep(elapsed);
                break;
            case BUZZ_LONG_BEEP:
                longBeep(elapsed);
                break;
            case BUZZ_SOS:
                sosPattern(elapsed);
                break;
            case BUZZ_STARTUP_OK:
                startupTone(elapsed);
                break;
            default:
                stop();
                break;
        }
    }

    void stop() {
        if (buzzerPin >= 0) digitalWrite(buzzerPin, LOW);
        currentPattern = BUZZ_NONE;
        isActive = false;
    }

    bool isBusy() const { return isActive; }

private:
    int buzzerPin;
    BuzzerPattern currentPattern;
    int patternStep;
    uint32_t lastStepTime;
    bool isActive;

    void beepOn() { digitalWrite(buzzerPin, HIGH); }
    void beepOff() { digitalWrite(buzzerPin, LOW); }

    void singleBeep(uint32_t elapsed) {
        if (elapsed < 100) beepOn();
        else { beepOff(); stop(); }
    }

    void doubleBeep(uint32_t elapsed) {
        if (elapsed < 80) beepOn();
        else if (elapsed < 160) beepOff();
        else if (elapsed < 240) beepOn();
        else { beepOff(); stop(); }
    }

    void tripleBeep(uint32_t elapsed) {
        if (elapsed < 80) beepOn();
        else if (elapsed < 160) beepOff();
        else if (elapsed < 240) beepOn();
        else if (elapsed < 320) beepOff();
        else if (elapsed < 400) beepOn();
        else { beepOff(); stop(); }
    }

    void longBeep(uint32_t elapsed) {
        if (elapsed < 500) beepOn();
        else { beepOff(); stop(); }
    }

    void sosPattern(uint32_t elapsed) {
        // S: ...  O: ---  S: ...
        int phase = (elapsed / 100) % 18;
        if (phase < 1 || (phase >= 2 && phase < 3) || (phase >= 4 && phase < 5)) beepOn(); // S
        else if (phase >= 6 && phase < 9) beepOn();   // O (long)
        else if (phase >= 10 && phase < 11) beepOn();  // second O
        else if (phase >= 12 && phase < 15) beepOn();  // third O
        else beepOff();
        
        if (elapsed > 2000) stop(); // Stop after one cycle
    }

    void startupTone(uint32_t elapsed) {
        if (elapsed < 50) beepOn();
        else if (elapsed < 100) beepOff();
        else if (elapsed < 150) beepOn();
        else if (elapsed < 200) beepOff();
        else if (elapsed < 300) beepOn();
        else { beepOff(); stop(); }
    }
};

#endif // BUZZER_SERVICE_H
