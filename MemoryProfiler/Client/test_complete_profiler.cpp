// test_complete_profiler.cpp
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QRandomGenerator>
#include <QVector>
#include <QString>
#include <QHash>
#include <QDateTime>
#include <algorithm>

#include "ServerClient.h"
#include "profiler_structures.h"

// Variables globales para el estado
double currentMemory = 100.0;
double memoryTrend = 1.0;
int metricsCount = 0;
int timelineCount = 0;
int topFilesCount = 0;
int memoryMapCount = 0;
int totalSeconds = 0;

// Lista de archivos para TopFiles
QVector<QString> allFiles = {
    "main.cpp", "renderer.cpp", "physics_engine.cpp", "network_manager.cpp",
    "asset_loader.cpp", "ui_manager.cpp", "audio_system.cpp", "shader_compiler.cpp"};

// Estado inicial de los archivos
QHash<QString, quint64> fileAllocations;
QHash<QString, double> fileMemory;

// Estado del memory map - SIMULA ASIGNACIONES REALES
QVector<MemoryMapTypes::BasicMemoryBlock> memoryBlocks;
quint64 nextAddress = 0x10000000;

// SIMULACIÓN DE CONTEXTOS Y LEAKS
struct Context {
    int id;
    QVector<quint64> allocatedBlocks; // Direcciones asignadas en este contexto
    bool active;
};
QVector<Context> contexts;
int currentContextId = 0;
int contextLeakCounter = 0;

Client *globalClient = nullptr;

void initializeFileData()
{
    fileAllocations["main.cpp"] = 1500;
    fileAllocations["renderer.cpp"] = 4500;
    fileAllocations["physics_engine.cpp"] = 3200;
    fileAllocations["network_manager.cpp"] = 2800;
    fileAllocations["asset_loader.cpp"] = 5100;
    fileAllocations["ui_manager.cpp"] = 1900;
    fileAllocations["audio_system.cpp"] = 1200;
    fileAllocations["shader_compiler.cpp"] = 800;

    fileMemory["main.cpp"] = 45.5;
    fileMemory["renderer.cpp"] = 128.7;
    fileMemory["physics_engine.cpp"] = 89.3;
    fileMemory["network_manager.cpp"] = 67.8;
    fileMemory["asset_loader.cpp"] = 156.2;
    fileMemory["ui_manager.cpp"] = 52.1;
    fileMemory["audio_system.cpp"] = 38.4;
    fileMemory["shader_compiler.cpp"] = 25.6;
}

// Simular entrada a un nuevo contexto/función
void enterNewContext() {
    Context newContext;
    newContext.id = ++currentContextId;
    newContext.active = true;
    contexts.append(newContext);
    qDebug() << "🚀 ENTRANDO a contexto" << currentContextId;
}

// Simular salida de contexto - DETECCIÓN DE LEAKS
void exitContext(int contextId) {
    for (auto &context : contexts) {
        if (context.id == contextId && context.active) {
            context.active = false;

            // SIMULAR DETECCIÓN DE LEAKS: 20% de bloques no liberados se marcan como leaks
            int leaksDetected = 0;
            for (quint64 addr : context.allocatedBlocks) {
                if (QRandomGenerator::global()->bounded(100) < 20) { // 20% probabilidad de leak
                    for (auto &block : memoryBlocks) {
                        if (block.address == addr && block.state == "active") {
                            block.state = "leak";
                            leaksDetected++;
                            qDebug() << "🚨 LEAK DETECTADO al salir de contexto" << contextId
                                     << "- Addr: 0x" << QString::number(addr, 16)
                                     << "File:" << block.filename;
                            break;
                        }
                    }
                }
            }

            qDebug() << "🔚 SALIENDO de contexto" << contextId
                     << "- Leaks detectados:" << leaksDetected
                     << "de" << context.allocatedBlocks.size() << "bloques";
            contextLeakCounter += leaksDetected;
            break;
        }
    }
}

// Asignar bloque en contexto actual
void allocateInCurrentContext(quint64 address) {
    if (!contexts.isEmpty() && contexts.last().active) {
        contexts.last().allocatedBlocks.append(address);
    }
}

void sendGeneralMetrics()
{
    if (!globalClient)
        return;

    metricsCount++;

    GeneralMetrics metrics;

    // Simular uso de memoria realista con tendencia
    double variation = (QRandomGenerator::global()->generateDouble() - 0.5) * 20.0;
    currentMemory = qMax(50.0, currentMemory + memoryTrend * 2.0 + variation);

    // Cambiar tendencia ocasionalmente
    if (QRandomGenerator::global()->bounded(100) < 20)
    {
        memoryTrend = (QRandomGenerator::global()->generateDouble() - 0.5) * 2.0;
    }

    // Calcular leaks basado en bloques marcados como leak
    double leakedMemoryMB = 0.0;
    for (const auto &block : memoryBlocks) {
        if (block.state == "leak") {
            leakedMemoryMB += block.size / (1024.0 * 1024.0);
        }
    }

    metrics.currentUsageMB = currentMemory;
    metrics.activeAllocations = 500 + QRandomGenerator::global()->bounded(1500);
    metrics.memoryLeaksMB = leakedMemoryMB;
    metrics.maxMemoryMB = qMax(metrics.maxMemoryMB, currentMemory + 20.0);
    metrics.totalAllocations = 5000 + QRandomGenerator::global()->bounded(45000);

    globalClient->send("GENERAL_METRICS", metrics);

    qDebug() << "📊 [" << metricsCount << "] GENERAL_METRICS -"
             << "Memoria:" << QString::number(metrics.currentUsageMB, 'f', 2) << "MB,"
             << "Activas:" << metrics.activeAllocations << ","
             << "Leaks:" << QString::number(metrics.memoryLeaksMB, 'f', 2) << "MB";
}

void sendTimelinePoint()
{
    if (!globalClient)
        return;

    timelineCount++;

    TimelinePoint point;
    point.timestamp = QDateTime::currentMSecsSinceEpoch();

    // Pequeña variación alrededor del valor actual de memoria
    double timelineVariation = (QRandomGenerator::global()->generateDouble() - 0.5) * 10.0;
    point.memoryMB = qMax(40.0, currentMemory + timelineVariation);

    globalClient->send("TIMELINE_POINT", point);

    qDebug() << "📈 [" << timelineCount << "] TIMELINE_POINT -"
             << "Memoria:" << QString::number(point.memoryMB, 'f', 2) << "MB";
}

void sendTopFiles()
{
    if (!globalClient)
        return;

    topFilesCount++;

    // Incrementar asignaciones de forma realista
    for (const QString &filename : allFiles)
    {
        fileAllocations[filename] += 50 + QRandomGenerator::global()->bounded(100);
        fileMemory[filename] += 0.5 + QRandomGenerator::global()->bounded(2.0);
    }

    // Ocasionalmente dar un boost a algún archivo aleatorio
    if (topFilesCount % 3 == 0)
    {
        QString boostedFile = allFiles[QRandomGenerator::global()->bounded(allFiles.size())];
        fileAllocations[boostedFile] += 1000;
        fileMemory[boostedFile] += 25.0;
    }

    // Crear vector de todos los archivos y ordenar por asignaciones
    QVector<TopFile> allTopFiles;
    for (const QString &filename : allFiles)
    {
        TopFile topFile;
        topFile.filename = filename;
        topFile.allocations = fileAllocations[filename];
        topFile.memoryMB = fileMemory[filename];
        allTopFiles.append(topFile);
    }

    // Ordenar por memoria descendente
    std::sort(allTopFiles.begin(), allTopFiles.end(),
              [](const TopFile &a, const TopFile &b)
              {
                  return a.memoryMB > b.memoryMB;
              });

    // Tomar top 3
    QVector<TopFile> topFiles;
    for (int i = 0; i < 3 && i < allTopFiles.size(); ++i)
    {
        topFiles.append(allTopFiles[i]);
    }

    globalClient->send("TOP_FILES", topFiles);

    qDebug() << "📁 [" << topFilesCount << "] TOP_FILES -"
             << "Archivos:" << topFiles.size()
             << "- Mayor:" << topFiles[0].filename << "(" << QString::number(topFiles[0].memoryMB, 'f', 1) << "MB)";
}

void sendMemoryMap()
{
    if (!globalClient)
        return;

    memoryMapCount++;

    // Archivos y tipos realistas para simulación
    const char *fileNames[] = {"renderer.cpp", "physics.cpp", "main.cpp", "utils.cpp", "network.cpp"};
    const char *types[] = {"new", "malloc", "calloc", "new[]"};

    // SIMULACIÓN DE ASIGNACIONES DINÁMICAS - HISTORIAL EN TIEMPO REAL
    int actionType = QRandomGenerator::global()->bounded(100);

    if (actionType < 60) {
        // 60% de probabilidad: Nueva asignación
        MemoryMapTypes::BasicMemoryBlock newBlock;
        newBlock.address = nextAddress;
        nextAddress += 0x1000; // Incrementar para siguiente bloque

        newBlock.size = 64 * (QRandomGenerator::global()->bounded(10) + 1) * 1024;

        // Tipo aleatorio
        int typeIndex = QRandomGenerator::global()->bounded(4);
        newBlock.type = types[typeIndex];
        newBlock.state = "active";

        // Archivo y línea
        int fileIndex = QRandomGenerator::global()->bounded(5);
        newBlock.filename = fileNames[fileIndex];
        newBlock.line = 100 + QRandomGenerator::global()->bounded(500);

        memoryBlocks.append(newBlock);
        allocateInCurrentContext(newBlock.address);

        qDebug() << "➕ NUEVA ASIGNACIÓN en contexto" << currentContextId
                 << "- Addr:" << QString("0x%1").arg(newBlock.address, 16, 16, QChar('0'))
                 << "Size:" << newBlock.size << "Type:" << newBlock.type;

    } else if (actionType < 85 && !memoryBlocks.isEmpty()) {
        // 25% de probabilidad: Liberar bloque existente
        int blockIndex = QRandomGenerator::global()->bounded(memoryBlocks.size());
        if (memoryBlocks[blockIndex].state == "active") {
            memoryBlocks[blockIndex].state = "freed";
            qDebug() << "➖ LIBERACIÓN - Addr:" << QString("0x%1").arg(memoryBlocks[blockIndex].address, 16, 16, QChar('0'));
        }
    }
    // Los leaks se detectan automáticamente al salir de contextos

    // Enviar todos los bloques actuales (historial completo)
    globalClient->send("BASIC_MEMORY_MAP", memoryBlocks);

    // Contar estadísticas para el log
    int activeCount = 0, freedCount = 0, leakCount = 0;
    for (const auto &block : memoryBlocks)
    {
        if (block.state == "active") activeCount++;
        else if (block.state == "freed") freedCount++;
        else if (block.state == "leak") leakCount++;
    }

    qDebug() << "🗺️  [" << memoryMapCount << "] BASIC_MEMORY_MAP -"
             << "Total:" << memoryBlocks.size()
             << "- Activos:" << activeCount
             << "- Liberados:" << freedCount
             << "- Leaks:" << leakCount;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    Client client;
    globalClient = &client;

    quint16 port = 8080;
    if (argc > 1)
        port = QString(argv[1]).toUShort();

    qDebug() << "================================================";
    qDebug() << "🧠 MEMORY PROFILER - PRUEBA COMPLETA CON LEAKS REALISTAS";
    qDebug() << "================================================";
    qDebug() << "Conectando al servidor en localhost:" << port;

    if (!client.connectToServer("localhost", port))
    {
        qDebug() << "❌ ERROR: No se pudo conectar al servidor";
        qDebug() << "   Asegúrate de que el servidor GUI esté ejecutándose en el puerto" << port;
        return -1;
    }

    qDebug() << "✅ Conectado al servidor correctamente";
    qDebug() << "📊 Iniciando envío de datos de prueba...";
    qDebug() << "";
    qDebug() << "⏰ Intervalos de envío:";
    qDebug() << "   - GENERAL_METRICS    cada 2 segundos";
    qDebug() << "   - TIMELINE_POINT     cada 1 segundo";
    qDebug() << "   - TOP_FILES          cada 5 segundos";
    qDebug() << "   - BASIC_MEMORY_MAP   cada 3 segundos";
    qDebug() << "   - CAMBIO CONTEXTO    cada 8 segundos";
    qDebug() << "";
    qDebug() << "🔍 DETECCIÓN DE LEAKS:";
    qDebug() << "   - Se simulan contextos (funciones/bloques de código)";
    qDebug() << "   - Al salir de contexto, 20% de bloques se marcan como leaks";
    qDebug() << "   - Los leaks se acumulan en el historial";
    qDebug() << "";

    // Inicializar datos de archivos
    initializeFileData();

    // Iniciar primer contexto
    enterNewContext();

    // Timer principal
    QTimer mainTimer;
    QObject::connect(&mainTimer, &QTimer::timeout, [&]()
                     {
                         totalSeconds++;

                         // Enviar TimelinePoint cada segundo
                         sendTimelinePoint();

                         // Enviar GeneralMetrics cada 2 segundos
                         if (totalSeconds % 2 == 0) {
                             sendGeneralMetrics();
                         }

                         // Enviar TopFiles cada 5 segundos
                         if (totalSeconds % 5 == 0) {
                             sendTopFiles();
                         }

                         // Enviar MemoryMap cada 3 segundos
                         if (totalSeconds % 3 == 0) {
                             sendMemoryMap();
                         }

                         // Cambiar de contexto cada 8 segundos - DETECCIÓN DE LEAKS
                         if (totalSeconds % 8 == 0 && totalSeconds > 0) {
                             // Salir del contexto actual (detectar leaks)
                             exitContext(currentContextId);
                             // Entrar a nuevo contexto
                             enterNewContext();
                         }

                         // Detener después de 60 segundos
                         if (totalSeconds >= 60) {
                             qDebug() << "\n================================================";
                             qDebug() << "✅ PRUEBA COMPLETADA";
                             qDebug() << "================================================";
                             qDebug() << "Resumen de envíos:";
                             qDebug() << "   - GENERAL_METRICS:   " << metricsCount;
                             qDebug() << "   - TIMELINE_POINT:    " << timelineCount;
                             qDebug() << "   - TOP_FILES:         " << topFilesCount;
                             qDebug() << "   - BASIC_MEMORY_MAP:  " << memoryMapCount;
                             qDebug() << "   - Contextos ejecutados:" << currentContextId;
                             qDebug() << "   - Leaks detectados:    " << contextLeakCounter;
                             qDebug() << "   - Bloques en historial:" << memoryBlocks.size();
                             qDebug() << "================================================";

                             mainTimer.stop();
                             client.disconnectFromServer();
                             QTimer::singleShot(1000, &app, &QCoreApplication::quit);
                         } });

    // Envío inicial
    QTimer::singleShot(100, [&]()
                       {
                           qDebug() << "🚀 Envío inicial de datos...";

                           // Enviar un conjunto inicial de todos los tipos
                           sendGeneralMetrics();
                           sendTimelinePoint();
                           sendTopFiles();
                           sendMemoryMap();

                           // Iniciar timer principal
                           mainTimer.start(1000); });

    qDebug() << "";
    qDebug() << "✅ Prueba en ejecución...";
    qDebug() << "   Duración: 1 minuto";
    qDebug() << "   Presiona Ctrl+C para detener antes";
    qDebug() << "================================================";

    return app.exec();
}
