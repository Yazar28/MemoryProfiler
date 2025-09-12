#include "MemoryTracker.h"
#include "MTDebug.h"
#include <iostream>
#include <chrono>
#include <utility>

static thread_local bool g_mt_in_tracker = false;
struct ReentryGuard {
    bool prev;
    ReentryGuard() : prev(g_mt_in_tracker) { g_mt_in_tracker = true; }
    ~ReentryGuard() { g_mt_in_tracker = prev; }
};

std::atomic<bool> MemoryTracker::alive{ false };

MemoryTracker::MemoryTracker() {
    alive.store(true, std::memory_order_relaxed);
}

MemoryTracker& MemoryTracker::getInstance() {
    static MemoryTracker instance;
    return instance;
}

MemoryTracker::~MemoryTracker() {
    MT_LOGLN("[TRK] ~MemoryTracker");
    alive.store(false, std::memory_order_relaxed);
    reportLeaks();
}

bool MemoryTracker::isAlive() noexcept {
    return alive.load(std::memory_order_relaxed);
}

void MemoryTracker::registerAllocation(void* ptr, size_t size, const char* file, int line, const char* type) {
    if (!ptr) return;
    if (g_mt_in_tracker) return;
    ReentryGuard guard;

    std::lock_guard<std::mutex> lock(mtx);

    AllocationInfo info;
    info.address = ptr;
    info.size = size;
    info.file = file ? file : "unknown";
    info.line = line;
    info.typeName = type ? type : "unknown";
    info.timestamp = std::chrono::high_resolution_clock::now();

    allocations[ptr] = std::move(info);

    ++totalAllocations;
    ++activeAllocations;
    currentMemory += size;
    if (currentMemory > peakMemory) peakMemory = currentMemory;

    MT_LOGLN("[TRK] new  ptr=" << ptr
        << " size=" << size
        << " @" << (file ? file : "unknown") << ":" << line);
}

void MemoryTracker::unregisterAllocation(void* ptr) {
    if (!ptr) return;
    if (g_mt_in_tracker) return;
    ReentryGuard guard;

    std::lock_guard<std::mutex> lock(mtx);

    auto it = allocations.find(ptr);
    if (it != allocations.end()) {
        currentMemory -= it->second.size;
        if (activeAllocations > 0) --activeAllocations;
        allocations.erase(it);
        MT_LOGLN("[TRK] del  ptr=" << ptr);
    }
}

void MemoryTracker::reportLeaks() {
    if (g_mt_in_tracker) return;
    ReentryGuard guard;

    std::lock_guard<std::mutex> lock(mtx);

    std::cout << "\n=== Memory Report ===\n";
    std::cout << "Total allocations: " << totalAllocations << "\n";
    std::cout << "Active allocations: " << activeAllocations << "\n";
    std::cout << "Peak memory usage: " << peakMemory << " bytes\n";
    std::cout << "Current memory: " << currentMemory << " bytes\n";

    if (allocations.empty()) {
        std::cout << "[MemoryTracker] No leaks detected.\n";
        return;
    }

    std::cout << "[MemoryTracker] Memory leaks detected (" << allocations.size() << "):\n";
    for (const auto& kv : allocations) {
        const auto& info = kv.second;
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            info.timestamp.time_since_epoch()
        ).count();

        std::cout << "  Leak at " << info.address
            << " | size: " << info.size
            << " | type: " << info.typeName
            << " | file: " << info.file << ":" << info.line
            << " | ts(ms): " << ms
            << "\n";
    }
}