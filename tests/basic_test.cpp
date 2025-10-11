#include <QCoreApplication>
#include <QTimer>
#include <QRandomGenerator>
#include <QDateTime>
#include "../Include/MemoryProfiler.h"
#include "../Client/ServerClient.h"

struct SimRun {
    int intervalMs = 150;   // velocidad de tick (grÃ¡fico principal)
    int bigEveryNTicks = 4; // cada cuÃ¡ntos ticks manda resumen/top/leaks/mapa
};

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    Client c;
    MemoryProfiler& mp = MemoryProfiler::instance();
    mp.setClient(&c);

    // Conectarse al server (GUI escucha 9099)
    auto doConnect = [&](){ c.connectToServer("127.0.0.1", 9099); };
    QObject::connect(&c, &Client::disconnected, &app, [&](){
        QTimer::singleShot(2000, &app, doConnect); // reintento
    });
    doConnect();

    // ====== SimulaciÃ³n externa (sin tocar la librerÃ­a) ======
    SimRun cfg;
    cfg.intervalMs     = 1000;  // sube/baja para ver la fluidez
    cfg.bigEveryNTicks = 4;    // 1 = manda todo en cada tick

    // Estado de simulaciÃ³n para timeline
    double currentMemory = 100.0;
    double trend = 0.0;
    quint64 tickCount = 0;

    // Active addresses (para free/leak directos sin pedirle a la librerÃ­a getters)
    QVector<quint64> activeAddrs;
    quint64 nextAddr = 0x10000000ULL;

    auto tick = [&](){
        if (!c.isConnected()) return;

        // === 1) Timeline principal ===
        ++tickCount;
        double variation = (QRandomGenerator::global()->generateDouble() - 0.5) * 20.0;
        currentMemory = qMax(50.0, currentMemory + trend * 2.0 + variation);
        if (QRandomGenerator::global()->bounded(100) < 20) {
            trend = (QRandomGenerator::global()->generateDouble() - 0.5) * 2.0;
        }

        // Var local como en la sim
        double tlVar = (QRandomGenerator::global()->generateDouble() - 0.5) * 10.0;
        mp.pushTimelinePoint(qMax(40.0, currentMemory + tlVar));

        // === 2) Eventos alloc/free/leak/realloc ===
        int action = QRandomGenerator::global()->bounded(100);
        if (action < 60) {
            // alloc
            quint64 addr = nextAddr; nextAddr += 0x1000ULL;
            quint64 size = 64ULL * (QRandomGenerator::global()->bounded(10) + 1) * 1024ULL;
            const char* files[] = { "renderer.cpp","physics.cpp","main.cpp","utils.cpp","network.cpp" };
            int idx = QRandomGenerator::global()->bounded(5);
            QString file = QString::fromLatin1(files[idx]);
            int line = 100 + QRandomGenerator::global()->bounded(500);
            mp.recordAlloc(addr, size, file, line,
                           size <= 1024ULL ? "small" : (size <= 1024ULL*1024ULL ? "medium" : "large"));
            activeAddrs.push_back(addr);
        }
        else if (action < 75) {
            // free
            if (!activeAddrs.isEmpty()) {
                int i = QRandomGenerator::global()->bounded(activeAddrs.size());
                quint64 addr = activeAddrs[i];
                mp.recordFree(addr);
                activeAddrs.remove(i);
            }
        }
        else if (action < 85) {
            // realloc (free + alloc doble)
            if (!activeAddrs.isEmpty()) {
                int i = QRandomGenerator::global()->bounded(activeAddrs.size());
                quint64 addr = activeAddrs[i];
                // simulamos como: free seguido de alloc misma addr doblando size
                mp.recordFree(addr);
                quint64 newSize = 128ULL * (QRandomGenerator::global()->bounded(10) + 1) * 1024ULL;
                mp.recordAlloc(addr, newSize, "renderer.cpp", 200, newSize <= 1024ULL ? "small" :
                               (newSize <= 1024ULL*1024ULL ? "medium" : "large"));
                // sigue activa (no la removemos)
            }
        }
        else {
            // leak directo
            if (!activeAddrs.isEmpty()) {
                int i = QRandomGenerator::global()->bounded(activeAddrs.size());
                quint64 addr = activeAddrs[i];
                mp.recordLeak(addr);
                activeAddrs.remove(i);
            }
        }

        // === 3) General metrics (desde el estado de la librerÃ­a) ===
        mp.sendGeneralFromState(currentMemory, currentMemory + 0.0); // max = clamp interno

        // === 4) Paquete â€œgrandeâ€ cada N ticks ===
        if ((tickCount % static_cast<quint64>(cfg.bigEveryNTicks)) == 0ULL) {
            mp.sendFileSummary();
            mp.updateLeakData();
            mp.sendTopFiles(8);
            // snapshot (si quieres ver el mapa completo)
            mp.sendBasicMemoryMapSnapshot();
        }
    };

    // Timer propio del TEST (no de la librerÃ­a)
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &app, tick);
    timer.start(cfg.intervalMs);

    // Mantener vivo (quita esto si quieres ciclo infinito)
    // QTimer::singleShot(5*60*1000, &app, &QCoreApplication::quit);

    return app.exec();
}