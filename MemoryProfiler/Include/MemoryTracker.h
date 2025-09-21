#pragma once
#include "AllocationInfo.h"
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>

class MemoryTracker {
public:
    struct Stats {
        size_t totalAllocations;
        size_t activeAllocations;
        size_t currentMemory;
        size_t peakMemory;
    };

    struct ReportEntry {
        void* address;
        size_t       size;
        std::string  file;
        int          line;
        std::string  typeName;
        long long    timestamp_ms;
    };

    struct Report {
        Stats stats;
        std::vector<ReportEntry> leaks;
    };

    // --- Singleton ---
    static MemoryTracker& getInstance();
    static bool isAlive() noexcept;
    static bool isInitializing() noexcept;

    // --- API ---
    void registerAllocation(void* ptr, size_t size, const char* file, int line, const char* type);
    void unregisterAllocation(void* ptr);
    Report collectReport();
    void reportLeaks();

    // --- Cctor/Dtor ---
    ~MemoryTracker();
    MemoryTracker(const MemoryTracker&) = delete;
    MemoryTracker& operator=(const MemoryTracker&) = delete;

private:
    MemoryTracker();

    // --- Estado ---
    std::unordered_map<void*, AllocationInfo> allocations;
    std::mutex mtx;

    size_t totalAllocations = 0;
    size_t activeAllocations = 0;
    size_t peakMemory = 0;
    size_t currentMemory = 0;

    static std::atomic<bool> alive;
    static std::atomic<bool> initializing;
};
