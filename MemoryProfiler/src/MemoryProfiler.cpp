#include "../Include/MemoryProfiler.h"
#include "../Client/ServerClient.h"        // tu Client real (sin tocar)
#include "../Client/profiler_structures.h" // structs de transporte

#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <algorithm>

MemoryProfiler& MemoryProfiler::instance() {
    static MemoryProfiler s;
    return s;
}

MemoryProfiler::MemoryProfiler(QObject* parent)
    : QObject(parent)
{
}

void MemoryProfiler::setClient(Client* c) { client_ = c; }

void MemoryProfiler::resetState() {
    blocks_.clear();
    indexByAddr_.clear();
    topCounts_.clear();
    topBytes_.clear();
    leakTimelineHistory_.clear();
    totalAllocs_ = 0;
    activeAllocs_ = 0;
}

/* =======================
 *  Ingesta de eventos externos
 * ======================= */

void MemoryProfiler::recordAlloc(quint64 address, quint64 size,
                                 const QString& filename, int line,
                                 const QString& typeDetail)
{
    // Actualizar estado interno
    BasicBlock nb;
    nb.address  = address;
    nb.size     = size;
    nb.type     = QStringLiteral("alloc");
    nb.state    = QStringLiteral("active");
    nb.filename = filename;
    nb.line     = line;
    blocks_.append(nb);
    indexByAddr_[address] = blocks_.size() - 1;

    // Contadores / Top Files
    ++totalAllocs_;
    ++activeAllocs_;
    topCounts_[filename] += 1;
    topBytes_[filename]  += size;

    // Evento de tiempo real
    sendMemoryEvent(QStringLiteral("alloc"), address, size, filename, line,
                    (typeDetail.isEmpty() ? QStringLiteral("alloc") : typeDetail));
}

void MemoryProfiler::recordFree(quint64 address, const QString& filename, int line)
{
    int idx = indexByAddr_.value(address, -1);
    if (idx >= 0 && idx < blocks_.size()) {
        auto& b = blocks_[idx];
        if (b.state == "active") {
            b.state = "freed";
            if (activeAllocs_ > 0) --activeAllocs_;

            sendMemoryEvent(QStringLiteral("free"), b.address, b.size,
                            filename.isEmpty() ? b.filename : filename,
                            line == 0 ? b.line : line,
                            QStringLiteral("free"));
        }
    }
}

void MemoryProfiler::recordLeak(quint64 address, const QString& filename, int line)
{
    int idx = indexByAddr_.value(address, -1);
    if (idx >= 0 && idx < blocks_.size()) {
        auto& b = blocks_[idx];
        if (b.state == "active") {
            b.state = "leak";
            if (activeAllocs_ > 0) --activeAllocs_;

            sendMemoryEvent(QStringLiteral("leak"), b.address, b.size,
                            filename.isEmpty() ? b.filename : filename,
                            line == 0 ? b.line : line,
                            QStringLiteral("leak"));
        }
    }
}

/* =======================
 *  Envío de métricas/gráficos
 * ======================= */

void MemoryProfiler::pushTimelinePoint(double memoryMB, quint64 timestampMs)
{
    if (!client_) return;
    TimelinePoint tp{};
    tp.timestamp = (timestampMs == 0)
                 ? static_cast<quint64>(QDateTime::currentMSecsSinceEpoch())
                 : timestampMs;
    tp.memoryMB  = memoryMB;
    client_->sendTimelinePoint(tp);
}

void MemoryProfiler::sendGeneralFromState(double currentUsageMB, double maxMemoryMB)
{
    if (!client_) return;

    // Derivar leaks MB desde el estado actual
    double leakedMB = 0.0;
    for (const auto& b : blocks_) {
        if (b.state == "leak") leakedMB += double(b.size) / (1024.0 * 1024.0);
    }

    GeneralMetrics gm{};
    gm.currentUsageMB    = currentUsageMB;
    gm.maxMemoryMB       = std::max(maxMemoryMB, currentUsageMB);
    gm.totalAllocations  = totalAllocs_;
    gm.activeAllocations = activeAllocs_;
    gm.memoryLeaksMB     = leakedMB;

    client_->sendGeneralMetrics(gm);
}

void MemoryProfiler::sendFileSummary()
{
    if (!client_) return;

    QHash<QString, quint64> allocCount;
    QHash<QString, quint64> totalBytes;
    QHash<QString, quint32> leakCount;
    QHash<QString, quint64> leakedBytes;

    for (const auto& b : blocks_) {
        if (b.type == "alloc") {
            allocCount[b.filename] += 1ULL;
            totalBytes[b.filename] += b.size;
        }
        if (b.state == "leak") {
            leakCount[b.filename]  += 1U;
            leakedBytes[b.filename]+= b.size;
        }
    }

    QList<FileAllocationSummary> files;
    files.reserve(allocCount.size());
    for (auto it = allocCount.begin(); it != allocCount.end(); ++it) {
        const QString& file = it.key();
        FileAllocationSummary s;
        s.filename          = file;
        s.allocationCount   = static_cast<quint64>(it.value());
        s.totalMemoryBytes  = static_cast<quint64>(totalBytes.value(file, 0));
        s.leakCount         = static_cast<quint32>(leakCount.value(file, 0));
        s.leakedMemoryBytes = static_cast<quint64>(leakedBytes.value(file, 0));
        s.memoryMB          = double(s.totalMemoryBytes) / (1024.0*1024.0);
        s.leakedMemoryMB    = double(s.leakedMemoryBytes) / (1024.0*1024.0);
        files.push_back(s);
    }
    client_->sendFileAllocationSummary(files);
}

void MemoryProfiler::updateLeakData()
{
    if (!client_) return;

    // Totales actuales
    double totalLeakedMB = 0.0;
    double biggestLeakMB = 0.0;
    QString biggestLeakLocation = QStringLiteral("Ninguno");
    QHash<QString,int>    leaksPerFile;
    QHash<QString,double> memPerFile;
    int totalLeakedBlocks = 0;

    for (const auto& b : blocks_) {
        if (b.state == "leak") {
            double mb = double(b.size) / (1024.0*1024.0);
            totalLeakedMB += mb;
            ++totalLeakedBlocks;
            leaksPerFile[b.filename] += 1;
            memPerFile[b.filename]   += mb;
            if (mb > biggestLeakMB) {
                biggestLeakMB = mb;
                biggestLeakLocation = QStringLiteral("%1:%2").arg(b.filename).arg(b.line);
            }
        }
    }

    // Más frecuente
    QString mostFreqFile = QStringLiteral("Ninguno");
    int maxLeaks = 0;
    for (auto it = leaksPerFile.begin(); it != leaksPerFile.end(); ++it) {
        if (it.value() > maxLeaks) { maxLeaks = it.value(); mostFreqFile = it.key(); }
    }

    // Summary
    {
        LeakSummary ls{};
        ls.totalLeakedMB        = totalLeakedMB;
        ls.biggestLeakMB        = biggestLeakMB;
        ls.biggestLeakLocation  = biggestLeakLocation;
        ls.mostFrequentLeakFile = mostFreqFile;
        ls.leakRate             = (totalAllocs_ > 0)
                                  ? (double(totalLeakedBlocks) * 100.0 / double(totalAllocs_))
                                  : 0.0;
        client_->sendLeakSummary(ls);
    }

    // By file
    {
        QList<LeakByFile> v;
        for (auto it = leaksPerFile.begin(); it != leaksPerFile.end(); ++it) {
            LeakByFile e;
            e.filename = it.key();
            e.leakCount = it.value();
            e.leakedMB  = memPerFile.value(it.key(), 0.0);
            v.push_back(e);
        }
        client_->sendLeaksByFile(v);
    }

    // Timeline acumulado (histórico)
    {
        const quint64 now = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
        leakTimelineHistory_.push_back(qMakePair(now, totalLeakedMB));
        if (leakTimelineHistory_.size() > maxTimelinePoints_) {
            leakTimelineHistory_.erase(
                leakTimelineHistory_.begin(),
                leakTimelineHistory_.begin() + (leakTimelineHistory_.size() - maxTimelinePoints_));
        }

        QList<LeakTimelinePoint> series;
        series.reserve(leakTimelineHistory_.size());
        for (const auto& pr : leakTimelineHistory_) {
            LeakTimelinePoint ltp;
            ltp.timestamp = pr.first;
            ltp.leakedMB  = pr.second;
            ltp.leakCount = 0; // pondremos el actual al final
            series.push_back(ltp);
        }
        if (!series.isEmpty()) {
            int currentBlocks = 0;
            for (const auto& b : blocks_) if (b.state == "leak") ++currentBlocks;
            series.back().leakCount = static_cast<quint64>(currentBlocks);
        }
        client_->sendLeakTimeline(series);
    }
}

void MemoryProfiler::sendTopFiles(size_t topN)
{
    if (!client_) return;

    struct Row { QString file; quint64 count = 0; quint64 bytes = 0; };
    QVector<Row> rows;
    rows.reserve(topCounts_.size());
    for (auto it = topCounts_.begin(); it != topCounts_.end(); ++it) {
        Row r{ it.key(), it.value(), topBytes_.value(it.key(), 0) };
        rows.push_back(r);
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b){
        if (a.count != b.count) return a.count > b.count;
        if (a.bytes != b.bytes) return a.bytes > b.bytes;
        return a.file < b.file;
    });
    if (rows.size() > static_cast<int>(topN)) rows.resize(static_cast<int>(topN));

    QVector<TopFile> out; out.reserve(rows.size());
    for (const Row& r : rows) {
        TopFile tf;
        tf.filename    = r.file;
        tf.allocations = r.count;
        tf.memoryMB    = double(r.bytes) / (1024.0 * 1024.0);
        out.push_back(tf);
    }
    client_->sendTopFiles(out);
}

void MemoryProfiler::sendBasicMemoryMapSnapshot()
{
    if (!client_) return;
    QList<MemoryMapTypes::BasicMemoryBlock> snap;
    snap.reserve(blocks_.size());
    for (const auto& b : blocks_) {
        MemoryMapTypes::BasicMemoryBlock bb;
        bb.address  = b.address;
        bb.size     = b.size;
        bb.type     = b.type;
        bb.state    = b.state;
        bb.filename = b.filename;
        bb.line     = b.line;
        snap.push_back(bb);
    }
    client_->sendBasicMemoryMap(snap);
}

/* =======================
 *  Helper para eventos “realtime”
 * ======================= */
void MemoryProfiler::sendMemoryEvent(const QString& eventType,
                                     quint64 address, quint64 size,
                                     const QString& filename, int line,
                                     const QString& typedetail)
{
    if (!client_) return;
    MemoryEvent ev;
    ev.address   = address;
    ev.size      = size;
    ev.event_type= eventType;     // "alloc"/"free"/"leak"
    ev.filename  = filename;
    ev.line      = line;
    ev.timestamp = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    ev.type      = typedetail;    // "small"/"medium"/"large" o "leak"
    client_->sendMemoryEvent(ev);
}