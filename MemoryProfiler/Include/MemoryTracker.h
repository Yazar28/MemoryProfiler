#pragma once
#include "AllocationInfo.h"
#include "ServerClient.h"
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>

class MemoryTracker
{
public:
    struct Stats
    {
        size_t totalAllocations;
        size_t activeAllocations;
        size_t currentMemory;
        size_t peakMemory;
    };

    struct ReportEntry
    {
        void *address;
        size_t size;
        std::string file;
        int line;
        std::string typeName;
        long long timestamp_ms;
    };

    struct Report
    {
        Stats stats;
        std::vector<ReportEntry> leaks;
    };

    struct FileSummary
    {
        std::string filename;
        size_t allocationCount;
        size_t totalMemory;
        size_t leakCount;
        size_t leakedMemory;
    };

    // --- Singleton ---
    static MemoryTracker &getInstance();
    static bool isAlive() noexcept;
    static bool isInitializing() noexcept;

    // --- API Principal ---
    void registerAllocation(void *ptr, size_t size, const char *file, int line, const char *type);
    void unregisterAllocation(void *ptr);

    // --- Reportes y Estadísticas ---
    Stats getCurrentStats();
    Report collectReport();
    void reportLeaks();
    std::vector<FileSummary> getFileSummaries();

    // --- API para Integración con Socket Client ---
    void enableRemoteReporting(const QString &host = "localhost", quint16 port = 8080);
    void disableRemoteReporting();
    bool isRemoteConnected() const;

    // --- Envío de Datos Específicos para la GUI ---
    void sendLiveUpdate(void *ptr, size_t size, bool isAlloc, const char *file, int line, const char *type);
    void sendGeneralMetrics();
    void sendMemoryMap();
    void sendFileAllocations();
    void sendLeakReport();
    void sendTimelinePoint();

    // --- Cctor/Dtor ---
    ~MemoryTracker();
    MemoryTracker(const MemoryTracker &) = delete;
    MemoryTracker &operator=(const MemoryTracker &) = delete;

private:
    MemoryTracker();
    void setupPeriodicUpdates();

    // --- Estado de Memoria ---
    std::unordered_map<void *, AllocationInfo> allocations;
    std::mutex mtx;

    size_t totalAllocations = 0;
    size_t activeAllocations = 0;
    size_t peakMemory = 0;
    size_t currentMemory = 0;
    size_t totalLeakedMemory = 0;

    // --- Integración con Client ---
    Client *socketClient = nullptr;
    bool remoteEnabled = false;

    // --- Para estadísticas periódicas ---
    QTimer *updateTimer = nullptr;

    static std::atomic<bool> alive;
    static std::atomic<bool> initializing;
};