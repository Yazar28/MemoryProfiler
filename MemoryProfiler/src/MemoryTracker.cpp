#include "MemoryTracker.h"
#include "MTDebug.h"
#include "ServerClient.h"

#include <QTimer>
#include <QDateTime>
#include <iostream>
#include <chrono>
#include <utility>
#include <sstream>
#include <algorithm>
#include <map>
#include <QVector>

//==================================================
// Anti-reentrada
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
    updateTimer  = nullptr;
    remoteEnabled = false;
}

MemoryTracker::~MemoryTracker()
{
    // Enviar reporte final antes de destruir
    if (remoteEnabled) {
        sendLeakReport();
    }

    if (updateTimer) {
        updateTimer->stop();
        delete updateTimer;
        updateTimer = nullptr;
    }

    if (socketClient) {
        delete socketClient;
        socketClient = nullptr;
    }

    alive.store(false, std::memory_order_release);
}

//==================================================
// Singleton
//==================================================
MemoryTracker &MemoryTracker::getInstance()
{
    static MemoryTracker *p = []() -> MemoryTracker * {
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

bool MemoryTracker::isAlive() noexcept            { return alive.load(std::memory_order_acquire); }
bool MemoryTracker::isInitializing() noexcept     { return initializing.load(std::memory_order_acquire); }

//==================================================
// Registro / Desregistro
//==================================================
void MemoryTracker::registerAllocation(void *ptr, size_t size, const char *file, int line, const char *type)
{
    if (!ptr) return;
    if (g_mt_in_tracker) return;
    ReentryGuard guard;

    std::lock_guard<std::mutex> lock(mtx);

    AllocationInfo info;
    info.address   = ptr;
    info.size      = size;
    info.file      = file ? std::string(file) : "unknown";
    info.line      = line;
    info.typeName  = type ? std::string(type) : "unknown";
    info.timestamp = std::chrono::high_resolution_clock::now();

    allocations[ptr] = std::move(info);

    ++totalAllocations;
    ++activeAllocations;
    currentMemory += size;
    if (currentMemory > peakMemory) peakMemory = currentMemory;

    // Envío en vivo desactivado (no hay API genérica en Client)
    // sendLiveUpdate(...);

    MT_LOGLN("[TRK] ALLOC ptr=" << ptr << " size=" << size << " @" << (file ? file : "unknown") << ":" << line);
}

void MemoryTracker::unregisterAllocation(void *ptr)
{
    if (!ptr) return;
    if (g_mt_in_tracker) return;
    ReentryGuard guard;

    std::lock_guard<std::mutex> lock(mtx);

    auto it = allocations.find(ptr);
    if (it != allocations.end()) {
        currentMemory -= it->second.size;
        if (activeAllocations > 0) --activeAllocations;
        allocations.erase(it);

        // Envío en vivo desactivado (no hay API genérica en Client)
        // sendLiveUpdate(...);

        MT_LOGLN("[TRK] FREE ptr=" << ptr);
    }
}

//==================================================
// Reportes y Estadísticas
//==================================================
MemoryTracker::Stats MemoryTracker::getCurrentStats()
{
    std::lock_guard<std::mutex> lock(mtx);
    return { totalAllocations, activeAllocations, currentMemory, peakMemory };
}

MemoryTracker::Report MemoryTracker::collectReport()
{
    ReentryGuard guard;
    std::lock_guard<std::mutex> lock(mtx);

    Report r;
    r.stats = { totalAllocations, activeAllocations, currentMemory, peakMemory };
    r.leaks.reserve(allocations.size());

    for (const auto &kv : allocations) {
        const auto &info = kv.second;
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(info.timestamp.time_since_epoch()).count();

        ReportEntry e;
        e.address      = info.address;
        e.size         = info.size;
        e.file         = info.file;
        e.line         = info.line;
        e.typeName     = info.typeName;
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

    for (const auto &kv : allocations) {
        const auto &info = kv.second;
        std::string filename = info.file.empty() ? "unknown" : info.file;

        if (fileMap.find(filename) == fileMap.end())
            fileMap[filename] = { filename, 0, 0, 0, 0 };

        fileMap[filename].allocationCount++;
        fileMap[filename].totalMemory  += info.size;
        fileMap[filename].leakCount++;
        fileMap[filename].leakedMemory += info.size;
    }

    std::vector<FileSummary> result;
    for (const auto &pair : fileMap)
        result.push_back(pair.second);

    std::sort(result.begin(), result.end(),
              [](const FileSummary &a, const FileSummary &b){ return a.totalMemory > b.totalMemory; });

    return result;
}

void MemoryTracker::reportLeaks()
{
#ifndef MT_SILENT_REPORT
    if (g_mt_in_tracker) return;
    ReentryGuard guard;

    if (remoteEnabled) {
        sendLeakReport();
    }

    Report r = collectReport();

    std::cout << "\n=== Memory Report ===\n";
    std::cout << "Total allocations: "  << r.stats.totalAllocations  << "\n";
    std::cout << "Active allocations: " << r.stats.activeAllocations << "\n";
    std::cout << "Peak memory usage: "  << r.stats.peakMemory        << " bytes\n";
    std::cout << "Current memory: "     << r.stats.currentMemory     << " bytes\n";

    if (r.leaks.empty()) {
        std::cout << "[MemoryTracker] No leaks detected.\n";
        return;
    }

    std::cout << "[MemoryTracker] Memory leaks detected (" << r.leaks.size() << "):\n";
    for (const auto &e : r.leaks) {
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
    if (g_mt_in_tracker) return;
    ReentryGuard guard;

    if (!socketClient)
        socketClient = new Client();

    remoteEnabled = true;

    if (socketClient->connectToServer(host, port)) {
        MT_LOGLN("[MT] Connected to remote server: " << host.toStdString() << ":" << port);
        setupPeriodicUpdates();
    } else {
        MT_LOGLN("[MT] Failed to connect to remote server");
    }
}

void MemoryTracker::disableRemoteReporting()
{
    remoteEnabled = false;
    if (updateTimer) updateTimer->stop();
}

bool MemoryTracker::isRemoteConnected() const
{
    return remoteEnabled && socketClient && socketClient->isConnected();
}

void MemoryTracker::setupPeriodicUpdates()
{
    if (!updateTimer) {
        updateTimer = new QTimer();
        QObject::connect(updateTimer, &QTimer::timeout, [this]() {
            if (!remoteEnabled) return;

            static int t = 0;
            sendGeneralMetrics();
            sendTimelinePoint();

            // Cada ~2s también mandamos mapa y resumen por archivo
            if (++t % 2 == 0) {
                sendMemoryMap();
                sendFileAllocations();
                sendLeakReport(); // opcional
                sendTopFiles();   // NUEVO: mismo “batch” que el resto
            }
        });
    }
    updateTimer->start(1000); // 1 Hz base
}


//==================================================
// Envío de Datos (usar API específica de Client)
//==================================================

// En tu Client NO existe "send(keyword, payload)". Desactivamos live-update.
void MemoryTracker::sendLiveUpdate(void*, size_t, bool, const char*, int, const char*)
{
    // No-op por ahora
}

void MemoryTracker::sendGeneralMetrics()
{
    if (!isRemoteConnected()) return;

    GeneralMetrics m{};
    // Ejemplo simple: reflejar contadores actuales
    {
        std::lock_guard<std::mutex> lock(mtx);
        m.currentUsageMB    = static_cast<double>(currentMemory) / (1024.0 * 1024.0);
        m.maxMemoryMB       = static_cast<double>(peakMemory)    / (1024.0 * 1024.0);
        m.activeAllocations = static_cast<quint32>(activeAllocations);
        m.totalAllocations  = static_cast<quint64>(totalAllocations);
        m.memoryLeaksMB     = static_cast<double>(totalLeakedMemory) / (1024.0 * 1024.0);
    }
    socketClient->sendGeneralMetrics(m);
}

void MemoryTracker::sendTimelinePoint()
{
    if (!isRemoteConnected()) return;

    TimelinePoint p{};
    p.timestamp = QDateTime::currentMSecsSinceEpoch();
    {
        std::lock_guard<std::mutex> lock(mtx);
        p.memoryMB = static_cast<double>(currentMemory) / (1024.0 * 1024.0);
    }
    socketClient->sendTimelinePoint(p);
}

void MemoryTracker::sendMemoryMap()
{
    if (!isRemoteConnected()) return;

    QList<MemoryMapTypes::BasicMemoryBlock> blocks;

    {
        std::lock_guard<std::mutex> lock(mtx);
        blocks.reserve(static_cast<int>(allocations.size()));

        for (const auto &kv : allocations) {
            const AllocationInfo &inf = kv.second;

            MemoryMapTypes::BasicMemoryBlock b;
            b.address  = reinterpret_cast<quint64>(inf.address);
            b.size     = static_cast<quint32>(inf.size);
            b.type     = QStringLiteral("Allocated");
            b.state    = QStringLiteral("Allocated");
            b.filename = QString::fromStdString(inf.file.empty() ? "unknown" : inf.file);
            b.line     = static_cast<quint32>(inf.line);

            blocks.push_back(b);
        }
    }

    socketClient->sendBasicMemoryMap(blocks);
}

void MemoryTracker::sendFileAllocations()
{
    if (!isRemoteConnected()) return;

    // filename -> (allocCount, totalBytes, leakCount, leakedBytes)
    struct Agg { quint32 ac=0; quint64 tb=0; quint32 lc=0; quint64 lb=0; };

    QHash<QString, Agg> agg;

    {
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto &kv : allocations) {
            const AllocationInfo &inf = kv.second;
            const QString file = QString::fromStdString(inf.file.empty() ? "unknown" : inf.file);

            auto &a = agg[file];
            a.ac += 1;
            a.tb += static_cast<quint64>(inf.size);
            a.lc += 1;                 // todo lo que queda vivo se considera leak por ahora
            a.lb += static_cast<quint64>(inf.size);
        }
    }

    QList<FileAllocationSummary> out;
    out.reserve(agg.size());

    for (auto it = agg.begin(); it != agg.end(); ++it) {
        FileAllocationSummary s;
        s.filename          = it.key();
        s.allocationCount   = it.value().ac;
        s.totalMemoryBytes  = it.value().tb;
        s.leakCount         = it.value().lc;
        s.leakedMemoryBytes = it.value().lb;
        out.push_back(s);
    }

    socketClient->sendFileAllocationSummary(out);
}

void MemoryTracker::sendTopFiles(size_t topN)
{
    if (!isRemoteConnected()) return;

    struct Agg { quint64 count = 0; quint64 bytes = 0; };
    QHash<QString, Agg> agg;

    {
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto &kv : allocations) {
            const AllocationInfo &inf = kv.second;
            const QString file = QString::fromStdString(inf.file.empty() ? "unknown" : inf.file);
            auto &a = agg[file];
            a.count += 1;
            a.bytes += static_cast<quint64>(inf.size);
        }
    }

    struct Row { QString file; quint64 count; quint64 bytes; };
    QVector<Row> rows;
    rows.reserve(agg.size());
    for (auto it = agg.begin(); it != agg.end(); ++it) {
        rows.push_back({ it.key(), it.value().count, it.value().bytes });
    }

    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b){
        if (a.count != b.count) return a.count > b.count;
        if (a.bytes != b.bytes) return a.bytes > b.bytes;
        return a.file < b.file;
    });

    if (rows.size() > static_cast<int>(topN))
        rows.resize(static_cast<int>(topN));

    // ✅ QVector<TopFile>, no QList
    QVector<TopFile> out;
    out.reserve(rows.size());
    for (const Row& r : rows) {
        TopFile tf;
        tf.filename    = r.file;
        tf.allocations = r.count;
        tf.memoryMB    = static_cast<double>(r.bytes) / (1024.0 * 1024.0);
        out.push_back(tf);
    }

    socketClient->sendTopFiles(out); // ✅ firma del Client original
}

void MemoryTracker::sendLeakReport()
{
    if (!isRemoteConnected()) return;

    LeakSummary sum{};
    {
        std::lock_guard<std::mutex> lock(mtx);
        sum.totalLeakedMB = static_cast<double>(totalLeakedMemory) / (1024.0 * 1024.0);
    }
    socketClient->sendLeakSummary(sum);

    // Leaks por archivo (lo vivo al final)
    QList<LeakByFile> byFile;
    {
        std::lock_guard<std::mutex> lock(mtx);
        QHash<QString, LeakByFile> map;
        for (const auto &kv : allocations) {
            const AllocationInfo &inf = kv.second;
            QString f = QString::fromStdString(inf.file.empty() ? "unknown" : inf.file);

            auto &e = map[f];
            e.filename = f;
            e.leakCount += 1;
            // Tus estructuras usan leakedMB (double). Convertimos bytes -> MB.
            e.leakedMB += static_cast<double>(inf.size) / (1024.0 * 1024.0);
        }

        byFile = map.values();
    }
    socketClient->sendLeaksByFile(byFile);

    // Timeline (un punto “now”; puedes añadir más si llevas histórico)
    LeakTimelinePoint pt{};
    pt.timestamp = QDateTime::currentMSecsSinceEpoch();
    pt.leakedMB  = sum.totalLeakedMB;
    socketClient->sendLeakTimeline(QList<LeakTimelinePoint>{ pt });
}
