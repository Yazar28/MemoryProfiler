#include <QCoreApplication>
#include <QTimer>
#include <QDateTime>
#include <QDirIterator>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QTextStream>

#include "../MemoryProfiler/Include/MemoryProfiler.h"
#include "../Client/ServerClient.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // --- Config directa dentro del main ---
    struct SimRun { int intervalMs; int bigEveryNTicks; };
    SimRun cfg;
    cfg.intervalMs     = 1000;  // sube/baja para ver la fluidez
    cfg.bigEveryNTicks = 4;

    // --- Ruta base desde argv[1] ---
    QString rootPath;
    if (argc > 1) {
        rootPath = QString::fromLocal8Bit(argv[1]);
    } else {
        // Si no pasas ruta, te pedimos una y salimos (evita recorrer todo el disco)
        QTextStream(stderr) << "[test_from_fs] Usage: test_from_fs <folder_to_scan>\n";
        return 1;
    }

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

    // --- Recorremos el directorio y cargamos una lista de archivos ---
    struct FEntry {
        QString filePath;     // ruta completa (para QFileInfo)
        QString displayFile;  // nombre lógico que verá la GUI (relativo)
        quint64 fileSize;     // en bytes
    };
    QVector<FEntry> files;
    files.reserve(1024);

    QDir root(rootPath);
    const int MAX_FILES = 500; // límite sanidad (ajústalo)
    if (root.exists()) {
        QDirIterator it(rootPath,
                        QStringList() << "*.cpp" << "*.c" << "*.h" << "*.hpp" << "*.txt" << "*.json" << "*.qml",
                        QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString f = it.next();
            QFileInfo info(f);
            if (!info.exists() || !info.isFile()) continue;

            FEntry e;
            e.filePath    = info.absoluteFilePath();
            e.displayFile = root.relativeFilePath(info.absoluteFilePath()); // relativo a root
            e.fileSize    = static_cast<quint64>(info.size());
            files.push_back(e);

            if (files.size() >= MAX_FILES) break;
        }
    }

    if (files.isEmpty()) {
        QTextStream(stderr) << "[test_from_fs] No files found in: " << rootPath << "\n";
        return 2;
    }

    // --- Estado de simulación ---
    quint64 tick = 0;
    double mem = 100.0;
    double trend = 0.0;

    // Map de “direcciones” sintéticas por archivo (varias allocs por archivo)
    struct AEntry { quint64 addr; quint64 size; QString file; int line; bool active; };
    QVector<AEntry> active; active.reserve(4096);
    quint64 nextAddr = 0x40000000ULL;

    auto allocFromFile = [&](const FEntry& fe) {
        // troceamos el archivo en “chunks” para simular varias allocs
        const quint64 chunk = 128ULL * 1024ULL;                 // 128KB por chunk
        const quint64 chunks = qMax<quint64>(1, fe.fileSize / chunk);
        const int linespread = 800;

        // Limitamos chunks por archivo para no inundar
        const quint64 maxChunks = qMin<quint64>(chunks, 8);
        for (quint64 i = 0; i < maxChunks; ++i) {
            const quint64 size = (i+1) * (chunk / maxChunks);   // tamaños crecientes
            quint64 addr = nextAddr; nextAddr += 0x1000ULL;
            int line = 10 + QRandomGenerator::global()->bounded(linespread);

            mp.recordAlloc(addr, size, fe.displayFile, line,
                           (size <= 128ULL*1024ULL) ? "small" : (size <= 512ULL*1024ULL ? "medium" : "large"));

            AEntry a{addr, size, fe.displayFile, line, true};
            active.push_back(a);
        }
    };

    auto freeRandom = [&](){
        if (active.isEmpty()) return;
        int i = QRandomGenerator::global()->bounded(active.size());
        if (!active[i].active) return;
        mp.recordFree(active[i].addr, active[i].file.toUtf8().constData(), active[i].line);
        active[i].active = false;
    };

    auto leakRandom = [&](){
        if (active.isEmpty()) return;
        int i = QRandomGenerator::global()->bounded(active.size());
        if (!active[i].active) return;
        mp.recordLeak(active[i].addr, active[i].file.toUtf8().constData(), active[i].line);
        active[i].active = false;
    };

    // Semilla inicial: toma un puñado de archivos y genera allocs base
    const int SEED_FILES = qMin(10, files.size());
    for (int i = 0; i < SEED_FILES; ++i) {
        allocFromFile(files[i]);
    }

    // Timer principal (corre constantemente)
    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&](){
        if (!c.isConnected()) return;
        ++tick;

        // Timeline suave
        double jitter = (QRandomGenerator::global()->generateDouble() - 0.5) * 8.0;
        mem = qMax(50.0, mem + trend * 1.5 + jitter);
        if (QRandomGenerator::global()->bounded(100) < 15) {
            trend = (QRandomGenerator::global()->generateDouble() - 0.5) * 2.0;
        }
        mp.pushTimelinePoint(mem);

        // Cada tick: elige aleatoriamente un archivo y genera allocs
        {
            const int idx = QRandomGenerator::global()->bounded(files.size());
            allocFromFile(files[idx]);
        }

        // También haz algunos frees/leaks al azar
        for (int i = 0; i < 2; ++i) freeRandom();
        for (int i = 0; i < 1; ++i) leakRandom();

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
