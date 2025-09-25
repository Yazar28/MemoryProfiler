#include "MemoryTracker.h"
#include "MTDebug.h"
#include "ServerClient.h"
#include <QTimer>
#include <iostream>
#include <chrono>
#include <utility>
#include <sstream>
#include <algorithm>
#include <map>

//==================================================
// Anti-reentrada
//==================================================
static thread_local bool g_mt_in_tracker = false;

struct ReentryGuard
{
    bool prev;
    ReentryGuard() : prev(g_mt_in_tracker) { g_mt_in_tracker = true; }
    ~ReentryGuard() { g_mt_in_tracker = prev; }
};

//==================================================
// Flags de estado del singleton
//==================================================
std::atomic<bool> MemoryTracker::alive{false};
std::atomic<bool> MemoryTracker::initializing{false};

//==================================================
// Constructor / Destructor
//==================================================
MemoryTracker::MemoryTracker()
{
    totalAllocations = 0;
    activeAllocations = 0;
    currentMemory = 0;
    peakMemory = 0;
    totalLeakedMemory = 0;
    socketClient = nullptr;
    updateTimer = nullptr;
}

MemoryTracker::~MemoryTracker()
{
    // Enviar reporte final antes de destruir
    if (remoteEnabled)
    {
        sendLeakReport();
    }

    if (updateTimer)
    {
        updateTimer->stop();
        delete updateTimer;
    }

    if (socketClient)
    {
        delete socketClient;
    }

    alive.store(false, std::memory_order_release);
}

//==================================================
// Singleton
//==================================================
MemoryTracker &MemoryTracker::getInstance()
{
    static MemoryTracker *p = []() -> MemoryTracker *
    {
        std::printf("[MT] getInstance: constructing...\n");
        std::fflush(stdout);
        initializing.store(true, std::memory_order_release);

        alignas(MemoryTracker) static unsigned char storage[sizeof(MemoryTracker)];
        auto inst = new (&storage) MemoryTracker();

        alive.store(true, std::memory_order_release);
        initializing.store(false, std::memory_order_release);

        std::printf("[MT] getInstance: constructed\n");
        std::fflush(stdout);
        return inst;
    }();
    return *p;
}

bool MemoryTracker::isAlive() noexcept
{
    return alive.load(std::memory_order_acquire);
}

bool MemoryTracker::isInitializing() noexcept
{
    return initializing.load(std::memory_order_acquire);
}

//==================================================
// Registro / Desregistro
//==================================================
void MemoryTracker::registerAllocation(void *ptr, size_t size, const char *file, int line, const char *type)
{
    if (!ptr)
        return;
    if (g_mt_in_tracker)
        return;
    ReentryGuard guard;

    std::lock_guard<std::mutex> lock(mtx);

    AllocationInfo info;
    info.address = ptr;
    info.size = size;
    info.file = file ? std::string(file) : "unknown";
    info.line = line;
    info.typeName = type ? std::string(type) : "unknown";
    info.timestamp = std::chrono::high_resolution_clock::now();

    allocations[ptr] = std::move(info);

    ++totalAllocations;
    ++activeAllocations;
    currentMemory += size;
    if (currentMemory > peakMemory)
        peakMemory = currentMemory;

    // Enviar actualización en tiempo real
    if (remoteEnabled)
    {
        sendLiveUpdate(ptr, size, true, file, line, type);
    }

    MT_LOGLN("[TRK] ALLOC ptr=" << ptr << " size=" << size << " @" << (file ? file : "unknown") << ":" << line);
}

void MemoryTracker::unregisterAllocation(void *ptr)
{
    if (!ptr)
        return;
    if (g_mt_in_tracker)
        return;
    ReentryGuard guard;

    std::lock_guard<std::mutex> lock(mtx);

    auto it = allocations.find(ptr);
    if (it != allocations.end())
    {
        currentMemory -= it->second.size;
        if (activeAllocations > 0)
            --activeAllocations;
        allocations.erase(it);

        // Enviar actualización en tiempo real
        if (remoteEnabled)
        {
            sendLiveUpdate(ptr, 0, false, "", 0, "");
        }

        MT_LOGLN("[TRK] FREE ptr=" << ptr);
    }
}

//==================================================
// Reportes y Estadísticas
//==================================================
MemoryTracker::Stats MemoryTracker::getCurrentStats()
{
    std::lock_guard<std::mutex> lock(mtx);
    return {totalAllocations, activeAllocations, currentMemory, peakMemory};
}

MemoryTracker::Report MemoryTracker::collectReport()
{
    ReentryGuard guard;
    std::lock_guard<std::mutex> lock(mtx);

    Report r;
    r.stats = {totalAllocations, activeAllocations, currentMemory, peakMemory};
    r.leaks.reserve(allocations.size());

    for (const auto &kv : allocations)
    {
        const auto &info = kv.second;
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            info.timestamp.time_since_epoch())
                            .count();

        ReportEntry e;
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

std::vector<MemoryTracker::FileSummary> MemoryTracker::getFileSummaries()
{
    ReentryGuard guard;
    std::lock_guard<std::mutex> lock(mtx);

    std::map<std::string, FileSummary> fileMap;

    for (const auto &kv : allocations)
    {
        const auto &info = kv.second;
        std::string filename = info.file.empty() ? "unknown" : info.file;

        if (fileMap.find(filename) == fileMap.end())
        {
            fileMap[filename] = {filename, 0, 0, 0, 0};
        }

        fileMap[filename].allocationCount++;
        fileMap[filename].totalMemory += info.size;
        fileMap[filename].leakCount++;
        fileMap[filename].leakedMemory += info.size;
    }

    std::vector<FileSummary> result;
    for (const auto &pair : fileMap)
    {
        result.push_back(pair.second);
    }

    // Ordenar por memoria total (descendente)
    std::sort(result.begin(), result.end(),
              [](const FileSummary &a, const FileSummary &b)
              {
                  return a.totalMemory > b.totalMemory;
              });

    return result;
}

void MemoryTracker::reportLeaks()
{
#ifndef MT_SILENT_REPORT
    if (g_mt_in_tracker)
        return;
    ReentryGuard guard;

    if (remoteEnabled)
    {
        sendLeakReport();
    }

    Report r = collectReport();

    std::cout << "\n=== Memory Report ===\n";
    std::cout << "Total allocations: " << r.stats.totalAllocations << "\n";
    std::cout << "Active allocations: " << r.stats.activeAllocations << "\n";
    std::cout << "Peak memory usage: " << r.stats.peakMemory << " bytes\n";
    std::cout << "Current memory: " << r.stats.currentMemory << " bytes\n";

    if (r.leaks.empty())
    {
        std::cout << "[MemoryTracker] No leaks detected.\n";
        return;
    }

    std::cout << "[MemoryTracker] Memory leaks detected (" << r.leaks.size() << "):\n";
    for (const auto &e : r.leaks)
    {
        std::cout << "  Leak at " << e.address
                  << " | size: " << e.size
                  << " | type: " << e.typeName
                  << " | file: " << e.file << ":" << e.line
                  << " | ts(ms): " << e.timestamp_ms
                  << "\n";
    }
#endif
}

//==================================================
// Integración con Socket Client
//==================================================
void MemoryTracker::enableRemoteReporting(const QString &host, quint16 port)
{
    if (g_mt_in_tracker)
        return;
    ReentryGuard guard;

    if (!socketClient)
    {
        socketClient = new Client();
    }

    remoteEnabled = true;

    if (socketClient->connectToServer(host, port))
    {
        MT_LOGLN("[MT] Connected to remote server: " << host.toStdString() << ":" << port);
        setupPeriodicUpdates();
    }
    else
    {
        MT_LOGLN("[MT] Failed to connect to remote server");
    }
}

void MemoryTracker::disableRemoteReporting()
{
    remoteEnabled = false;
    if (updateTimer)
    {
        updateTimer->stop();
    }
}

bool MemoryTracker::isRemoteConnected() const
{
    return remoteEnabled && socketClient && socketClient->isConnected();
}

void MemoryTracker::setupPeriodicUpdates()
{
    if (!updateTimer)
    {
        updateTimer = new QTimer();
        QObject::connect(updateTimer, &QTimer::timeout, [this]()
                         {
            if (remoteEnabled) {
                sendGeneralMetrics();
                sendTimelinePoint();
            } });
    }
    updateTimer->start(1000); // Actualizar cada segundo
}

//==================================================
// Envío de Datos para la GUI (Formato específico)
//==================================================
void MemoryTracker::sendLiveUpdate(void *ptr, size_t size, bool isAlloc, const char *file, int line, const char *type)
{
    if (!isRemoteConnected() || g_mt_in_tracker)
        return;

    std::stringstream data;
    if (isAlloc)
    {
        data << "ALLOC|"
             << reinterpret_cast<uintptr_t>(ptr) << "|"
             << size << "|"
             << (file ? file : "unknown") << "|"
             << line << "|"
             << (type ? type : "unknown");
    }
    else
    {
        data << "FREE|" << reinterpret_cast<uintptr_t>(ptr);
    }

    std::string dataStr = data.str();
    socketClient->send("LIVE_UPDATE", QByteArray(dataStr.c_str(), dataStr.size()));
}

void MemoryTracker::sendGeneralMetrics()
{
    if (!isRemoteConnected() || g_mt_in_tracker)
        return;

    auto stats = getCurrentStats();
    std::stringstream data;
    data << "METRICS|"
         << stats.totalAllocations << "|"
         << stats.activeAllocations << "|"
         << stats.currentMemory << "|"
         << stats.peakMemory << "|"
         << totalLeakedMemory;

    std::string dataStr = data.str();
    socketClient->send("GENERAL_METRICS", QByteArray(dataStr.c_str(), dataStr.size()));
}

void MemoryTracker::sendMemoryMap()
{
    if (!isRemoteConnected() || g_mt_in_tracker)
        return;

    std::lock_guard<std::mutex> lock(mtx);

    std::stringstream data;
    data << "MEMORY_MAP_START|" << allocations.size();

    for (const auto &kv : allocations)
    {
        const auto &info = kv.second;
        data << "|BLOCK|"
             << reinterpret_cast<uintptr_t>(info.address) << "|"
             << info.size << "|"
             << info.typeName << "|"
             << info.file << "|"
             << info.line;
    }

    data << "|MEMORY_MAP_END";

    std::string dataStr = data.str();
    socketClient->send("MEMORY_MAP", QByteArray(dataStr.c_str(), dataStr.size()));
}

void MemoryTracker::sendFileAllocations()
{
    if (!isRemoteConnected() || g_mt_in_tracker)
        return;

    auto summaries = getFileSummaries();
    std::stringstream data;
    data << "FILE_SUMMARY_START|" << summaries.size();

    for (const auto &summary : summaries)
    {
        data << "|FILE|"
             << summary.filename << "|"
             << summary.allocationCount << "|"
             << summary.totalMemory << "|"
             << summary.leakCount << "|"
             << summary.leakedMemory;
    }

    data << "|FILE_SUMMARY_END";

    std::string dataStr = data.str();
    socketClient->send("FILE_ALLOCATIONS", QByteArray(dataStr.c_str(), dataStr.size()));
}

void MemoryTracker::sendLeakReport()
{
    if (!isRemoteConnected() || g_mt_in_tracker)
        return;

    auto report = collectReport();
    auto fileSummaries = getFileSummaries();

    std::stringstream data;
    data << "LEAK_REPORT|"
         << report.leaks.size() << "|"
         << totalLeakedMemory << "|";

    // Leak más grande
    if (!report.leaks.empty())
    {
        auto maxLeak = *std::max_element(report.leaks.begin(), report.leaks.end(),
                                         [](const ReportEntry &a, const ReportEntry &b)
                                         { return a.size < b.size; });
        data << maxLeak.size << "|" << maxLeak.file << "|";
    }
    else
    {
        data << "0|none|";
    }

    // Archivo con más leaks
    if (!fileSummaries.empty())
    {
        data << fileSummaries[0].filename << "|" << fileSummaries[0].leakCount;
    }
    else
    {
        data << "none|0";
    }

    // Lista de leaks detallada
    data << "|LEAKS_START|" << report.leaks.size();
    for (const auto &leak : report.leaks)
    {
        data << "|LEAK|"
             << reinterpret_cast<uintptr_t>(leak.address) << "|"
             << leak.size << "|"
             << leak.file << "|"
             << leak.line << "|"
             << leak.typeName << "|"
             << leak.timestamp_ms;
    }
    data << "|LEAKS_END";

    std::string dataStr = data.str();
    socketClient->send("LEAK_REPORT", QByteArray(dataStr.c_str(), dataStr.size()));
}

void MemoryTracker::sendTimelinePoint()
{
    if (!isRemoteConnected() || g_mt_in_tracker)
        return;

    auto stats = getCurrentStats();
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();

    std::stringstream data;
    data << "TIMELINE|"
         << now << "|"
         << stats.currentMemory << "|"
         << stats.activeAllocations;

    std::string dataStr = data.str();
    socketClient->send("TIMELINE_POINT", QByteArray(dataStr.c_str(), dataStr.size()));
}