#include "./Include/MpLibSim.h"
#include <QDateTime>
#include <QRandomGenerator>
#include <QtMath>

MpLibSim::MpLibSim(QObject* parent)
    : QObject(parent)
{
    timer_.setInterval(tickMs_);
    connect(&timer_, &QTimer::timeout, this, &MpLibSim::tick);
}

void MpLibSim::start()
{
    if (!client_) return;

    counter_      = 0;
    currentMB_    = 50.0;
    targetMB_     = currentMB_;
    maxMB_        = currentMB_;
    totalAllocs_  = 0;
    activeAllocs_ = 0;

    t0EpochMs_ = QDateTime::currentMSecsSinceEpoch();
    lastMs_    = t0EpochMs_ - tickMs_;

    timer_.start();
}

void MpLibSim::stop()
{
    timer_.stop();
}

void MpLibSim::tick()
{
    if (!client_) return;
    ++counter_;

    double noise  = (QRandomGenerator::global()->bounded(2001) - 1000) / 1000.0; // [-1,1]
    targetMB_    += drift_ + noise * noiseAmp_;
    wavePhase_   += 0.22;
    targetMB_    += waveAmp_ * qSin(wavePhase_);

    if ((counter_ % (8 + QRandomGenerator::global()->bounded(4))) == 0) {
        targetMB_ += 2.8 + QRandomGenerator::global()->bounded(25) / 10.0;
    }

    targetMB_ = qBound(43.0, targetMB_, 108.0);

    double delta = targetMB_ - currentMB_;
    if (delta >  maxStepMB_) delta =  maxStepMB_;
    if (delta < -maxStepMB_) delta = -maxStepMB_;
    currentMB_ += delta;

    if (currentMB_ > maxMB_) maxMB_ = currentMB_;

    if ((counter_ % 3) == 0) { ++totalAllocs_; ++activeAllocs_; }
    if ((counter_ % 5) == 0 && activeAllocs_ > 0) { --activeAllocs_; }

    qint64 nowMs = lastMs_ + tickMs_;
    lastMs_ = nowMs;

    sendGeneral();
    sendTimeline(nowMs);

    if ((counter_ % (1000 / tickMs_)) == 0) { // ~1 Hz
        sendMemoryMap();
        sendFileSummary();
        sendLeakStuff();
    }
}

void MpLibSim::sendGeneral()
{
    GeneralMetrics gm{};
    gm.currentUsageMB    = currentMB_;
    gm.activeAllocations = static_cast<quint32>(activeAllocs_);
    gm.totalAllocations  = static_cast<quint64>(totalAllocs_);
    gm.maxMemoryMB       = maxMB_;
    gm.memoryLeaksMB     = 0.0;
    client_->sendGeneralMetrics(gm);
}

void MpLibSim::sendTimeline(qint64 nowMs)
{
    TimelinePoint tp{};
    tp.timestamp = static_cast<quint64>(nowMs);
    tp.memoryMB  = currentMB_;
    client_->sendTimelinePoint(tp);
}

void MpLibSim::sendMemoryMap()
{
    QList<MemoryMapTypes::BasicMemoryBlock> blocks;
    blocks.reserve(5);

    const quint64 baseAddr = 0x10002000ULL + (counter_ % 4) * 0x2000ULL;

    for (int i = 0; i < 5; ++i) {
        MemoryMapTypes::BasicMemoryBlock b;
        b.address  = baseAddr + static_cast<quint64>(i) * 0x2000ULL;
        b.size     = 0x4000 + i * 0x2000;
        b.type     = QStringLiteral("Allocated");
        b.state    = QStringLiteral("Allocated");
        b.filename = (i % 2 == 0) ? QStringLiteral("main.cpp")
                                  : QStringLiteral("engine/memory.cpp");
        b.line     = (i % 2 == 0) ? (10 + i * 14) : (31 + i * 28);
        blocks.push_back(b);
    }

    client_->sendBasicMemoryMap(blocks);
}

void MpLibSim::sendFileSummary()
{
    QList<FileAllocationSummary> files;

    {
        FileAllocationSummary s;
        s.filename          = QStringLiteral("main.cpp");
        s.allocationCount   = 2 + static_cast<quint32>(counter_ % 3);
        s.totalMemoryBytes  = 64 * 1024 + static_cast<quint64>(counter_ % 2) * 16 * 1024;
        s.leakCount         = 0;
        s.leakedMemoryBytes = 0;
        files.push_back(s);
    }
    {
        FileAllocationSummary s;
        s.filename          = QStringLiteral("engine/memory.cpp");
        s.allocationCount   = 3 + static_cast<quint32>(counter_ % 2);
        s.totalMemoryBytes  = 96 * 1024 + static_cast<quint64>(counter_ % 3) * 8 * 1024;
        s.leakCount         = 0;
        s.leakedMemoryBytes = 0;
        files.push_back(s);
    }

    client_->sendFileAllocationSummary(files);
}

void MpLibSim::sendLeakStuff()
{
    LeakSummary ls{};
    ls.totalLeakedMB = 1.25;
    client_->sendLeakSummary(ls);

    QList<LeakByFile> byFile;
    {
        LeakByFile e;
        e.filename  = QStringLiteral("main.cpp");
        e.leakCount = 2;
        e.leakedMB  = 0.03;
        byFile.push_back(e);
    }
    {
        LeakByFile e;
        e.filename  = QStringLiteral("engine/memory.cpp");
        e.leakCount = 1;
        e.leakedMB  = 0.008;
        byFile.push_back(e);
    }
    client_->sendLeaksByFile(byFile);

    QList<LeakTimelinePoint> ltp;
    LeakTimelinePoint p;
    p.timestamp = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    p.leakedMB  = 0.15;
    ltp.push_back(p);
    client_->sendLeakTimeline(ltp);
}
