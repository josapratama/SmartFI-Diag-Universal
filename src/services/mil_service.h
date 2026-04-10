/**
 * @file mil_service.h
 * @brief MIL Lamp Reader and Decoder.
 */

#ifndef MIL_SERVICE_H
#define MIL_SERVICE_H

#include <memory>
#include <vector>
#include "core/channel_manager.h"
#include "config_types.h"

class MILService {
public:
    MILService() : milChan(nullptr), lastState(false), blinkCount(0), lastBlinkTime(0) {}

    void init() {
        milChan = ChannelManager::getInstance().getDigital("CH_MIL");
    }

    void update(const VehicleProfile& profile) {
        if (!milChan || !profile.caps.has_mil) return;

        bool currentState = milChan->read();
        uint32_t now = millis();

        // Decode based on mil_mode
        if (profile.mil_mode == "PULSE_COUNT") {
            // Logic for pulse counting
        } else if (profile.mil_mode == "MORSE_CODE") {
            // Logic for short/long pulses (e.g. Honda MIL)
        }

        // Edge detection for blink decoding
        if (currentState != lastState) {
            // ... (rest of the logic)
            lastState = currentState;
        }
    }

    bool getStatus() const { return milChan ? milChan->read() : false; }
    int getBlinkCode() const { return blinkCount; }

private:
    std::shared_ptr<DigitalDriver> milChan;
    bool lastState;
    int blinkCount;
    uint32_t lastBlinkTime;
};

#endif // MIL_SERVICE_H
