#include <QCoreApplication>
#include <QTimer>
#include <QRandomGenerator>
#include <QDateTime>

#include "../MemoryProfiler/Include/MemoryProfiler.h"
#include "../Client/ServerClient.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // --- Config directa dentro del main ---
    struct SimRun { int intervalMs; int bigEveryNTicks; };
    SimRun cfg;
    cfg.intervalMs     = 1000;  // sube/baja para ver la fluidez
    cfg.bigEveryNTicks = 4;

    // --- Conexión al GUI ---
    Client c;
    auto doConnect = [&](){ c.connectToServer("127.0.0.1", 9099); };
    QObject::connect(&c, &Client::disconnected, &app, [&](){
        QTimer::singleShot(1500, &app, doConnect);
    });
    doConnect();

    MemoryProfiler& mp = MemoryProfiler::instance();
    mp.setClient(&c);
    mp.resetState();

    quint64 tick = 0;
    double mem = 120.0;
    double trend = 0.0;

    QVector<quint64> active;
    quint64 nextAddr = 0x20000000ULL;

    auto oneAlloc = [&](const QString& file) {
        quint64 addr = nextAddr; nextAddr += 0x1000ULL;
        quint64 size = (64ULL + QRandomGenerator::global()->bounded(1024ULL)) * 1024ULL; // ~64KB..1MB
        int line = 50 + QRandomGenerator::global()->bounded(900);
        mp.recordAlloc(addr, size, file, line,
            (size <= 128ULL*1024ULL) ? "small" : (size <= 512ULL*1024ULL ? "medium" : "large"));
        active.push_back(addr);
    };

    auto oneFree = [&](){
        if (active.isEmpty()) return;
        int i = QRandomGenerator::global()->bounded(active.size());
        quint64 addr = active[i];
        mp.recordFree(addr);
        active.remove(i);
    };

    auto oneLeak = [&](){
        if (active.isEmpty()) return;
        int i = QRandomGenerator::global()->bounded(active.size());
        quint64 addr = active[i];
        mp.recordLeak(addr);
        active.remove(i);
    };

    const QString files[] = { "renderer.cpp", "physics.cpp", "scene.cpp", "utils.cpp", "io/file.cpp" };

    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&](){
        if (!c.isConnected()) return;
        ++tick;

        // Timeline suave
        double jitter = (QRandomGenerator::global()->generateDouble() - 0.5) * 10.0;
        mem = qMax(60.0, mem + trend * 2.0 + jitter);
        if (QRandomGenerator::global()->bounded(100) < 20) {
            trend = (QRandomGenerator::global()->generateDouble() - 0.5) * 2.0;
        }
        mp.pushTimelinePoint(mem);

        // Carga
        oneAlloc(files[QRandomGenerator::global()->bounded(int(std::size(files)))]);
        oneAlloc(files[QRandomGenerator::global()->bounded(int(std::size(files)))]);
        oneAlloc(files[QRandomGenerator::global()->bounded(int(std::size(files)))]);
        oneFree();
        oneLeak();

        // General
        mp.sendGeneralFromState(mem, mem);

        // Bursts cada N ticks
        if ((tick % quint64(cfg.bigEveryNTicks)) == 0ULL) {
            mp.sendFileSummary();
            mp.updateLeakData();
            mp.sendTopFiles(10);
            mp.sendBasicMemoryMapSnapshot();
        }
    });
    t.start(cfg.intervalMs);  // usa tu intervalo

    return app.exec();        // corre indefinidamente
}
