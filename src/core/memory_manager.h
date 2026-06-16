/**
 * @file memory_manager.h
 * @brief Memory Manager for ESP32 - heap monitoring and fragmentation prevention.
 * 
 * Strategy:
 * - Use StaticJsonDocument for fixed-size, frequently-used JSON operations
 * - Use pre-allocated char buffers for JSON serialization output
 * - Monitor free heap and warn when low
 * - Provide safe allocation wrappers
 */

#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <Arduino.h>

class MemoryManager {
public:
    static MemoryManager& getInstance() {
        static MemoryManager instance;
        return instance;
    }

    void init() {
        initialFreeHeap = ESP.getFreeHeap();
        minFreeHeap = initialFreeHeap;
        Serial.printf("[Memory] Initial free heap: %u bytes\n", initialFreeHeap);
        Serial.printf("[Memory] Largest free block: %u bytes\n", ESP.getMaxAllocHeap());
    }

    /**
     * Call periodically (e.g., every 10 seconds) to monitor heap health.
     */
    void update() {
        uint32_t currentFree = ESP.getFreeHeap();
        
        if (currentFree < minFreeHeap) {
            minFreeHeap = currentFree;
        }

        // Warn if heap is getting low
        if (currentFree < HEAP_WARNING_THRESHOLD && !heapWarningIssued) {
            Serial.printf("[Memory] WARNING: Free heap low! %u bytes (min: %u)\n", 
                currentFree, minFreeHeap);
            heapWarningIssued = true;
        }
        
        // Critical: if heap is dangerously low, trigger cleanup
        if (currentFree < HEAP_CRITICAL_THRESHOLD) {
            Serial.printf("[Memory] CRITICAL: Free heap %u bytes! Triggering cleanup.\n", currentFree);
            triggerCleanup();
        }

        // Reset warning if heap recovers
        if (currentFree > HEAP_WARNING_THRESHOLD + 10000) {
            heapWarningIssued = false;
        }
    }

    uint32_t getFreeHeap() const { return ESP.getFreeHeap(); }
    uint32_t getMinFreeHeap() const { return minFreeHeap; }
    uint32_t getInitialFreeHeap() const { return initialFreeHeap; }
    uint32_t getLargestFreeBlock() const { return ESP.getMaxAllocHeap(); }
    
    float getFragmentationPercent() const {
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t largestBlock = ESP.getMaxAllocHeap();
        if (freeHeap == 0) return 100.0f;
        return (1.0f - ((float)largestBlock / (float)freeHeap)) * 100.0f;
    }

    bool isHeapHealthy() const {
        return ESP.getFreeHeap() > HEAP_WARNING_THRESHOLD;
    }

    bool canAllocate(size_t bytes) const {
        return ESP.getMaxAllocHeap() > bytes + 1024; // 1KB safety margin
    }

    // Cleanup callback - set by main to free non-essential caches
    typedef void (*CleanupCallback)();
    void setCleanupCallback(CleanupCallback cb) { cleanupCb = cb; }

private:
    MemoryManager() : initialFreeHeap(0), minFreeHeap(0), 
                      heapWarningIssued(false), cleanupCb(nullptr) {}

    uint32_t initialFreeHeap;
    uint32_t minFreeHeap;
    bool heapWarningIssued;
    CleanupCallback cleanupCb;

    static const uint32_t HEAP_WARNING_THRESHOLD = 30000;   // 30KB
    static const uint32_t HEAP_CRITICAL_THRESHOLD = 15000;  // 15KB

    void triggerCleanup() {
        if (cleanupCb) {
            cleanupCb();
        }
    }
};

// ==========================================
// PRE-ALLOCATED JSON BUFFERS
// ==========================================

/**
 * Static buffer pool for JSON serialization.
 * Avoids heap allocation for frequently-used JSON operations.
 */
namespace JsonBuffers {
    // Telemetry broadcast (used every 100ms)
    static char telemetryBuf[256];
    
    // Health score (used every 500ms)
    static char healthBuf[384];
    
    // Live data API response
    static char liveBuf[256];
}

#endif // MEMORY_MANAGER_H
