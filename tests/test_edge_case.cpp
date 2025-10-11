#include <QCoreApplication>
#include <QTimer>
#include <QDateTime>
#include <QVector>

#include "../MemoryProfiler/Include/MemoryProfiler.h"
#include "../Client/ServerClient.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // --- Config directa dentro del main ---
    struct SimRun { int intervalMs; int bigEveryNTicks; };
    SimRun cfg;
    cfg.intervalMs     = 1000;  // sube/baja para ver la fluidez
    cfg.bigEveryNTicks = 2;

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

    // Direcciones controladas
    const quint64 addrZero       = 0x30000000ULL; // alloc 0 bytes
    const quint64 addrHuge       = 0x30001000ULL; // alloc enorme
    const quint64 addrDoubleFree = 0x30002000ULL; // double free
    const quint64 addrUnknown    = 0xDEADBEEFULL; // free desconocido
    const quint64 addrLeak1      = 0x30003000ULL;
    const quint64 addrLeak2      = 0x30004000ULL;

    // 1) Timeline inicial
    mp.pushTimelinePoint(90.0);
    mp.sendGeneralFromState(90.0, 90.0);

    // 2) Edge cases una vez
    mp.recordAlloc(addrZero, 0, "edge/zero.cpp", 10, "small");
    mp.recordAlloc(addrHuge, 64ULL * 1024ULL * 1024ULL, "edge/huge.cpp", 20, "large");

    mp.recordAlloc(addrDoubleFree, 128ULL * 1024ULL, "edge/double_free.cpp", 30, "small");
    mp.recordFree(addrDoubleFree, "edge/double_free.cpp", 31);
    mp.recordFree(addrDoubleFree, "edge/double_free.cpp", 32); // no-op esperado

    mp.recordFree(addrUnknown, "edge/unknown.cpp", 40); // free desconocido

    mp.recordAlloc(addrLeak1, 256ULL * 1024ULL, "edge/leaks.cpp", 50, "small");
    mp.recordAlloc(addrLeak2, 384ULL * 1024ULL, "edge/leaks.cpp", 60, "small");
    mp.recordLeak(addrLeak1, "edge/leaks.cpp", 51);
    mp.recordLeak(addrLeak2, "edge/leaks.cpp", 61);

    // 3) Primer resumen
    mp.sendFileSummary();
    mp.updateLeakData();
    mp.sendTopFiles(8);
    mp.sendBasicMemoryMapSnapshot();

    // 4) Timeline vivo + refrescos periódicos
    double mem = 92.0;
    quint64 tick = 0;

    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&](){
        if (!c.isConnected()) return;
        ++tick;

        mem += (tick % 2 == 0) ? 0.35 : -0.22;
        mp.pushTimelinePoint(mem);
        mp.sendGeneralFromState(mem, mem);

        if ((tick % quint64(cfg.bigEveryNTicks)) == 0ULL) {
            mp.sendFileSummary();
            mp.updateLeakData();
            mp.sendTopFiles(8);
            mp.sendBasicMemoryMapSnapshot();
        }
    });
    t.start(cfg.intervalMs);  // usa tu intervalo

    return app.exec();        // corre indefinidamente
}
