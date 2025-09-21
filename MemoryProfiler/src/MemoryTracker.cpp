#include "MemoryTracker.h"
#include "MTDebug.h"
#include <iostream>
#include <chrono>
#include <utility>
#include <sstream>
#include <atomic>
#include <mutex>
#include <new>      // placement-new
#include <cstdio>   // printf/fflush

//==================================================
// Anti-reentrada dentro del tracker
//==================================================
static thread_local bool g_mt_in_tracker = false;

struct ReentryGuard {
    bool prev;
    ReentryGuard() : prev(g_mt_in_tracker) { g_mt_in_tracker = true; }
    ~ReentryGuard() { g_mt_in_tracker = prev; }
};

//==================================================
// Flags de estado del singleton
//==================================================
std::atomic<bool> MemoryTracker::alive{ false };
std::atomic<bool> MemoryTracker::initializing{ false };

//==================================================
// Constructor / Destructor (sin heap ni I/O pesado)
//==================================================
MemoryTracker::MemoryTracker() {
    totalAllocations = 0;
    activeAllocations = 0;
    currentMemory = 0;
    peakMemory = 0;
    // unordered_map/mutex no deberían alocar en el ctor por defecto
}

MemoryTracker::~MemoryTracker() {
    // reportLeaks(); // opcional, desactivado para evitar I/O tardío
    alive.store(false, std::memory_order_release);
}

//==================================================
// Singleton sin heap (placement-new) y a prueba de reentradas
//==================================================
MemoryTracker& MemoryTracker::getInstance() {
    static MemoryTracker* p = []() -> MemoryTracker* {
        std::printf("[MT] getInstance: constructing...\n"); std::fflush(stdout);

        // Avisar que estamos construyendo (bloquea registros de new/delete)
        initializing.store(true, std::memory_order_release);

        // Almacenamiento estático para evitar heap
        alignas(MemoryTracker) static unsigned char storage[sizeof(MemoryTracker)];
        auto inst = new (&storage) MemoryTracker();

        // Ya está construido: marcar vivo y limpiar bandera de inicialización
        alive.store(true, std::memory_order_release);
        initializing.store(false, std::memory_order_release);

        std::printf("[MT] getInstance: constructed\n"); std::fflush(stdout);
        return inst;
        }();
    return *p;
}

bool MemoryTracker::isAlive() noexcept {
    return alive.load(std::memory_order_acquire);
}

bool MemoryTracker::isInitializing() noexcept {
    return initializing.load(std::memory_order_acquire);
}

//==================================================
// Registro / desregistro
//==================================================
void MemoryTracker::registerAllocation(void* ptr, size_t size, const char* file, int line, const char* type) {
    if (!ptr) return;
    if (g_mt_in_tracker) return;
    ReentryGuard guard;

    std::lock_guard<std::mutex> lock(mtx);

    AllocationInfo info;
    info.address = ptr;
    info.size = size;
    info.file = file ? std::string(file) : std::string("unknown");
    info.line = line;
    info.typeName = type ? std::string(type) : std::string("unknown");
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

//==================================================
// Reporte
//==================================================
MemoryTracker::Report MemoryTracker::collectReport() {
    // 👇 **Clave**: bloquear re-entrada mientras reservamos/push_back en vectores
    ReentryGuard guard;

    std::lock_guard<std::mutex> lock(mtx);

    Report r;
    r.stats = { totalAllocations, activeAllocations, currentMemory, peakMemory };
    r.leaks.reserve(allocations.size());

    for (const auto& kv : allocations) {
        const auto& info = kv.second;
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            info.timestamp.time_since_epoch()
        ).count();

        MemoryTracker::ReportEntry e;
        e.address = info.address;
        e.size = info.size;
        e.file = info.file;
        e.line = info.line;
        e.typeName = info.typeName;
        e.timestamp_ms = ms;

        r.leaks.push_back(std::move(e));
    }
    return r;
}

void MemoryTracker::reportLeaks() {
#ifndef MT_SILENT_REPORT
    if (g_mt_in_tracker) return;
    ReentryGuard guard;

    Report r = collectReport();

    std::cout << "\n=== Memory Report ===\n";
    std::cout << "Total allocations: " << r.stats.totalAllocations << "\n";
    std::cout << "Active allocations: " << r.stats.activeAllocations << "\n";
    std::cout << "Peak memory usage: " << r.stats.peakMemory << " bytes\n";
    std::cout << "Current memory: " << r.stats.currentMemory << " bytes\n";

    if (r.leaks.empty()) {
        std::cout << "[MemoryTracker] No leaks detected.\n";
        return;
    }

    std::cout << "[MemoryTracker] Memory leaks detected (" << r.leaks.size() << "):\n";
    for (const auto& e : r.leaks) {
        std::cout << "  Leak at " << e.address
            << " | size: " << e.size
            << " | type: " << e.typeName
            << " | file: " << e.file << ":" << e.line
            << " | ts(ms): " << e.timestamp_ms
            << "\n";
    }
#endif
}
