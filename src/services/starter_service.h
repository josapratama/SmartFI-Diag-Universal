/**
 * @file starter_service.h
 * @brief Starter Monitoring and Cranking Analysis.
 */

#ifndef STARTER_SERVICE_H
#define STARTER_SERVICE_H

#include <memory>
#include "core/channel_manager.h"
#include "config_types.h"

class StarterService {
public:
    StarterService() : starterChan(nullptr), minCrankVolt(12.6f), crankDuration(0), startCrankTime(0) {}

    void init() {
        starterChan = ChannelManager::getInstance().getDigital("CH_STR");
    }

    void update(const VehicleProfile& profile, float battery_v) {
        if (!starterChan || !profile.caps.has_starter_signal) return;

        bool isCranking = starterChan->read();
        
        if (isCranking) {
            if (startCrankTime == 0) {
                startCrankTime = millis();
                minCrankVolt = battery_v;
            }
            if (battery_v < minCrankVolt) minCrankVolt = battery_v;
            crankDuration = millis() - startCrankTime;
        } else {
            if (startCrankTime > 0) {
                // End of cranking event
                startCrankTime = 0;
            }
        }
    }

    bool getCrankingStatus() const { return starterChan ? starterChan->read() : false; }
    float getMinCrankVolt() const { return minCrankVolt; }
    uint32_t getCrankDuration() const { return crankDuration; }

private:
    std::shared_ptr<DigitalDriver> starterChan;
    float minCrankVolt;
    uint32_t crankDuration;
    uint32_t startCrankTime;
};

#endif // STARTER_SERVICE_H
