#pragma once
#include "AllocationInfo.h"
#include <unordered_map>
#include <mutex>
#include <atomic>

class MemoryTracker {
public:
    static MemoryTracker& getInstance();
    static bool isAlive() noexcept;

    void registerAllocation(void* ptr, size_t size, const char* file, int line, const char* type = "unknown");
    void unregisterAllocation(void* ptr);
    void reportLeaks();

private:
    MemoryTracker();
    ~MemoryTracker();
    MemoryTracker(const MemoryTracker&) = delete;
    MemoryTracker& operator=(const MemoryTracker&) = delete;

    std::unordered_map<void*, AllocationInfo> allocations;
    std::mutex mtx;

    size_t totalAllocations = 0;
    size_t activeAllocations = 0;
    size_t peakMemory = 0;
    size_t currentMemory = 0;

    static std::atomic<bool> alive;
};