#include <QCoreApplication>
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QTextStream>
#include <QVector>
#include <QStringList>

#include "../MemoryProfiler/Include/MemoryProfiler.h"   // AJUSTA si tu ruta difiere
#include "../Client/ServerClient.h"                      // AJUSTA si tu ruta difiere

// Une argv[1..] para tolerar rutas con espacios (sin depender de comillas)
static QString joinArgsAsPath(int argc, char** argv) {
    QStringList parts;
    for (int i = 1; i < argc; ++i) parts << QString::fromLocal8Bit(argv[i]);
    return parts.join(' ');
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // ===== 1) Argumento: carpeta a escanear =====
    QString rootPath;
    if (argc > 1) {
        rootPath = joinArgsAsPath(argc, argv);
    } else {
        QTextStream(stderr)
            << "[test_from_fs] Usage: test_from_fs <folder_to_scan>\n"
            << "Ejemplo: \"C:/Users/Usuario/.../MemoryProfiler/MemoryProfiler/src\"\n";
        return 1;
    }

    // ===== 2) Conexión a la GUI en 127.0.0.1:9099 (reconecta si cae) =====
    Client c;
    auto doConnect = [&](){ c.connectToServer("127.0.0.1", 9099); };
    QObject::connect(&c, &Client::disconnected, &app, [&](){
        QTimer::singleShot(1500, &app, doConnect);
    });
    doConnect();

    // ===== 3) Inicializa el profiler (usa la API real de tu clase) =====
    MemoryProfiler& mp = MemoryProfiler::instance();
    mp.setClient(&c);
    mp.resetState();

    // ===== 4) Verificación de la ruta (siempre visible) =====
    {
        QFileInfo fi(rootPath);
        QTextStream out(stdout);
        out << "[test_from_fs] rootPath = " << rootPath << "\n";
        out << "[test_from_fs] exists?  = " << (fi.exists() ? "true" : "false") << "\n";
        out << "[test_from_fs] isDir?   = " << (fi.isDir() ? "true" : "false") << "\n";
        out << "[test_from_fs] absPath  = " << QDir(rootPath).absolutePath() << "\n";
        out.flush();
        if (!fi.exists()) {
            QTextStream(stderr) << "[test_from_fs] ERROR: la ruta NO existe.\n";
        } else if (!fi.isDir()) {
            QTextStream(stderr) << "[test_from_fs] ERROR: la ruta NO es un directorio.\n";
        }
    }

    // ===== 5) Escaneo tolerante por filtros =====
    struct FEntry {
        QString filePath;     // ruta completa
        QString displayFile;  // relativa para mostrar en GUI
        quint64 fileSize;     // bytes
    };
    QVector<FEntry> files;
    const int MAX_FILES = 500;

    auto scanWith = [&](const QStringList& filters, const char* tag){
        int count = 0;
        QTextStream out(stdout);

        QDirIterator it(rootPath, filters, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString f = it.next();
            QFileInfo info(f);
            if (!info.exists() || !info.isFile()) continue;

            FEntry e;
            e.filePath    = info.absoluteFilePath();
            e.displayFile = QDir(rootPath).relativeFilePath(info.absoluteFilePath());
            e.fileSize    = static_cast<quint64>(info.size());
            files.push_back(e);

            if (count < 5) { out << "[test_from_fs] " << tag << " hit: " << e.filePath << "\n"; out.flush(); }
            ++count;
            if ((int)files.size() >= MAX_FILES) break;
        }
        QTextStream(stdout) << "[test_from_fs] " << tag << " count = " << count << "\n";
    };

    scanWith({"*.cpp","*.c","*.cc"},     "SRC");
    if (files.size() < 50) scanWith({"*.h","*.hpp"},    "HDR");
    if (files.size() < 50) scanWith({"*.txt","*.json"}, "TXT");
    if (files.size() < 50) scanWith({"*.qml"},          "QML");
    if (files.isEmpty())  scanWith(QStringList{},       "ANY");

    // ===== 6) Estado de asignaciones activas (con soporte de leaks) =====
    struct AEntry {
        quint64 addr;
        quint64 size;
        QString file;
        int     line;
        bool    active;
        bool    leaked;  // <<--- importante para no liberar fugas
    };
    QVector<AEntry> active;
    quint64 nextAddr = 0x10000000ULL;

    // ===== 7) Generadores =====
    auto allocFromFile = [&](const FEntry& fe) {
        // troceo del archivo en hasta 8 chunks para simular varias allocs
        const quint64 chunk     = 128ULL * 1024ULL;                     // 128 KiB
        const quint64 chunks    = qMax<quint64>(1, fe.fileSize / chunk);
        const quint64 maxChunks = qMin<quint64>(chunks, 8);
        const int     linespread = 800;

        for (quint64 i = 0; i < maxChunks; ++i) {
            const quint64 size = (i + 1) * (chunk / maxChunks);
            const quint64 addr = (nextAddr += 0x1000ULL);
            const int line = 10 + QRandomGenerator::global()->bounded(linespread);

            mp.recordAlloc(addr, size, fe.displayFile, line,
                           (size <= 128ULL*1024ULL) ? "small"
                           : (size <= 512ULL*1024ULL ? "medium" : "large"));

            active.push_back(AEntry{addr, size, fe.displayFile, line, true, false});
        }
    };

    auto freeRandom = [&](){
        if (active.isEmpty()) return;
        const int i = QRandomGenerator::global()->bounded(active.size());
        if (!active[i].active || active[i].leaked) return; // ¡no liberar fugas!
        mp.recordFree(active[i].addr);
        active[i].active = false;
    };

    auto leakRandom = [&](){
        if (active.isEmpty()) return;
        // busca una entrada activa que aún no sea leak
        for (int tries = 0; tries < 5; ++tries) {
            const int i = QRandomGenerator::global()->bounded(active.size());
            if (active[i].active && !active[i].leaked) {
                mp.recordLeak(active[i].addr);     // <-- marca fuga real
                active[i].leaked = true;
                QTextStream(stdout) << "[test_from_fs] LEAK marked at addr=0x"
                                    << QString::number(active[i].addr, 16)
                                    << " file=" << active[i].file
                                    << " line=" << active[i].line << "\n";
                return;
            }
        }
    };

    // ===== 8) Bucle de envío periódico a la GUI =====
    struct { int intervalMs = 1000; int bigEveryNTicks = 2; } cfg;
    quint64 tick = 0;
    double mem = 120.0;
    double trend = 0.0;

    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&](){
        if (!c.isConnected()) return;
        ++tick;

        // Timeline suave con jitter
        double jitter = (QRandomGenerator::global()->generateDouble() - 0.5) * 10.0;
        mem = qMax(60.0, mem + trend * 2.0 + jitter);
        if (QRandomGenerator::global()->bounded(100) < 20) {
            trend = (QRandomGenerator::global()->generateDouble() - 0.5) * 2.0;
        }
        mp.pushTimelinePoint(mem);

        // Generar actividad
        if (!files.isEmpty()) {
            const int idx = QRandomGenerator::global()->bounded(files.size());
            allocFromFile(files[idx]);
        } else {
            // actividad mínima sintética si no hay archivos
            const QString fake = "synthetic.cpp";
            const quint64 addr = (nextAddr += 0x1000ULL);
            const quint64 size = (64ULL + QRandomGenerator::global()->bounded(1024ULL)) * 1024ULL;
            const int line = 100 + QRandomGenerator::global()->bounded(300);
            mp.recordAlloc(addr, size, fake, line,
                           (size <= 128ULL*1024ULL) ? "small"
                           : (size <= 512ULL*1024ULL ? "medium" : "large"));
            active.push_back(AEntry{addr, size, fake, line, true, false});
        }

        // Liberaciones y fugas
        if ((tick % 3ULL)  == 0ULL) freeRandom();
        if ((tick % 11ULL) == 0ULL) leakRandom();  // <-- genera leaks periódicamente

        // Métricas generales
        mp.sendGeneralFromState(mem, mem);

        // Bursts: resumen, leaks y mapa
        if ((tick % quint64(cfg.bigEveryNTicks)) == 0ULL) {
            mp.sendFileSummary();
            mp.updateLeakData();              // <-- refresca datos de fugas para la GUI
            mp.sendTopFiles(10);
            mp.sendBasicMemoryMapSnapshot();
        }
    });
    t.start(cfg.intervalMs);

    // ===== 9) (Opcional) Fuga garantizada al inicio =====
    QTimer::singleShot(200, &app, [&](){
        const QString fake = "forced_leak.cpp";
        const quint64 addr = (nextAddr += 0x1000ULL);
        const quint64 size = 256ULL * 1024ULL; // 256 KiB
        const int line = 777;
        mp.recordAlloc(addr, size, fake, line, "forced");
        active.push_back(AEntry{addr, size, fake, line, true, false});
        mp.recordLeak(addr);                   // <-- marcada como fuga
        active.back().leaked = true;
        QTextStream(stdout) << "[test_from_fs] Forced LEAK at addr=0x"
                            << QString::number(addr,16) << " size=" << size << "B\n";
    });

    return app.exec();
}
