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
struct Context
{
    int id;
    QVector<quint64> allocatedBlocks; // Direcciones asignadas en este contexto
    bool active;
};
QVector<Context> contexts;
int currentContextId = 0;
int contextLeakCounter = 0;
Client *globalClient = nullptr;

// Contadores específicos para leaks
int forcedLeaksCreated = 0;
int normalAllocations = 0;
int totalFreed = 0;

// 🆕 NUEVO: Contador de eventos de leak enviados
int leakEventsSent = 0;

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
void enterNewContext()
{
    Context newContext;
    newContext.id = ++currentContextId;
    newContext.active = true;
    contexts.append(newContext);
    qDebug() << "🚀 ENTRANDO a contexto" << currentContextId;
}

// Simular salida de contexto - DETECCIÓN DE LEAKS
void exitContext(int contextId)
{
    for (auto &context : contexts)
    {
        if (context.id == contextId && context.active)
        {
            context.active = false;

            // 🔴 FORZAR LEAKS: 60% de probabilidad de leak en este contexto
            int leaksDetected = 0;
            for (quint64 addr : context.allocatedBlocks)
            {
                if (QRandomGenerator::global()->bounded(100) < 60) // 60% de leaks!
                {
                    for (auto &block : memoryBlocks)
                    {
                        if (block.address == addr && block.state == "active")
                        {
                            block.state = "leak"; // 🆕 CAMBIO: "leak" en lugar de "leaked"
                            leaksDetected++;
                            forcedLeaksCreated++;

                            // 🆕 ENVIAR EVENTO DE LEAK ESPECÍFICO
                            if (globalClient) {
                                MemoryEvent leakEvent;
                                leakEvent.address = block.address;
                                leakEvent.size = block.size;
                                leakEvent.event_type = "leak"; // 🆕 Tipo específico para leak
                                leakEvent.filename = block.filename;
                                leakEvent.line = block.line;
                                leakEvent.timestamp = QDateTime::currentMSecsSinceEpoch();
                                leakEvent.type = "leak";

                                globalClient->sendMemoryEvent(leakEvent);
                                leakEventsSent++;

                                qDebug() << "🔴🆕 EVENTO LEAK ENVIADO - Addr: 0x"
                                         << QString::number(block.address, 16)
                                         << "Size:" << block.size << "bytes";
                            }

                            qDebug() << "🔴🆕 LEAK FORZADO en contexto" << contextId
                                     << "- Addr: 0x" << QString::number(addr, 16)
                                     << "Size:" << block.size << "bytes"
                                     << "File:" << block.filename;
                            break;
                        }
                    }
                }
            }

            qDebug() << "🔚 SALIENDO de contexto" << contextId
                     << "- LEAKS FORZADOS:" << leaksDetected
                     << "de" << context.allocatedBlocks.size() << "bloques";
            contextLeakCounter += leaksDetected;
            break;
        }
    }
}

// Asignar bloque en contexto actual
void allocateInCurrentContext(quint64 address)
{
    if (!contexts.isEmpty() && contexts.last().active)
    {
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
    int activeBlocks = 0;
    int leakedBlocks = 0;

    for (const auto &block : memoryBlocks)
    {
        if (block.state == "active") {
            activeBlocks++;
        } else if (block.state == "leak") { // 🆕 CAMBIO: "leak" en lugar de "leaked"
            leakedBlocks++;
            leakedMemoryMB += block.size / (1024.0 * 1024.0);
        }
    }

    metrics.currentUsageMB = currentMemory;
    metrics.activeAllocations = activeBlocks;
    metrics.memoryLeaksMB = leakedMemoryMB;
    metrics.maxMemoryMB = qMax(metrics.maxMemoryMB, currentMemory + 20.0);
    metrics.totalAllocations = normalAllocations;

    globalClient->sendGeneralMetrics(metrics);

    qDebug() << "📊 [" << metricsCount << "] GENERAL_METRICS -"
             << "Memoria:" << QString::number(metrics.currentUsageMB, 'f', 2) << "MB,"
             << "Activas:" << metrics.activeAllocations
             << "Leaks:" << leakedBlocks << "bloques,"
             << QString::number(metrics.memoryLeaksMB, 'f', 2) << "MB";
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

    globalClient->sendTimelinePoint(point);

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

    globalClient->sendTopFiles(topFiles);

    qDebug() << "📁 [" << topFilesCount << "] TOP_FILES -"
             << "Archivos:" << topFiles.size()
             << "- Mayor:" << topFiles[0].filename << "(" << QString::number(topFiles[0].memoryMB, 'f', 1) << "MB)";
}

void sendMemoryEvent(const QString &eventType, quint64 address, quint64 size, const QString &filename, int line)
{
    if (!globalClient)
        return;

    MemoryEvent event;
    event.address = address;
    event.size = size;
    event.event_type = eventType;
    event.filename = filename;
    event.line = line;
    event.timestamp = QDateTime::currentMSecsSinceEpoch();

    // Tipo específico basado en tamaño/contexto
    if (size <= 1024)
        event.type = "small";
    else if (size <= 1024 * 1024)
        event.type = "medium";
    else
        event.type = "large";

    globalClient->sendMemoryEvent(event);

    if (eventType == "alloc") {
        qDebug() << "🟢 ALLOC - Addr: 0x" << QString::number(address, 16)
                 << "Size:" << size << "bytes -" << filename << ":" << line;
    } else if (eventType == "free") {
        qDebug() << "🔵 FREE  - Addr: 0x" << QString::number(address, 16)
                 << "Size:" << size << "bytes";
    } else if (eventType == "leak") {
        qDebug() << "🔴🆕 LEAK EVENT - Addr: 0x" << QString::number(address, 16)
                 << "Size:" << size << "bytes -" << filename << ":" << line;
    }
}

// 🆕 NUEVA FUNCIÓN: Enviar evento de leak específico
void sendLeakEvent(quint64 address, quint64 size, const QString &filename, int line)
{
    if (!globalClient)
        return;

    MemoryEvent leakEvent;
    leakEvent.address = address;
    leakEvent.size = size;
    leakEvent.event_type = "leak"; // 🆕 Tipo específico para leak
    leakEvent.filename = filename;
    leakEvent.line = line;
    leakEvent.timestamp = QDateTime::currentMSecsSinceEpoch();
    leakEvent.type = "leak"; // 🆕 Tipo leak

    globalClient->sendMemoryEvent(leakEvent);
    leakEventsSent++;

    qDebug() << "🔴🎯 EVENTO LEAK ENVIADO - Addr: 0x" << QString::number(address, 16)
             << "Size:" << size << "bytes -" << filename << ":" << line;
}

void sendMemoryMap()
{
    if (!globalClient)
        return;

    memoryMapCount++;

    // Archivos y tipos realistas para simulación
    const char *fileNames[] = {"renderer.cpp", "physics.cpp", "main.cpp", "utils.cpp", "network.cpp"};

    // SIMULAR EVENTOS INDIVIDUALES EN TIEMPO REAL
    int actionType = QRandomGenerator::global()->bounded(100);

    if (actionType < 60) // 🔴 60% de probabilidad de NUEVA ASIGNACIÓN
    {
        // Nueva asignación
        quint64 address = nextAddress;
        nextAddress += 0x1000;
        quint64 size = 64 * (QRandomGenerator::global()->bounded(10) + 1) * 1024;
        int fileIndex = QRandomGenerator::global()->bounded(5);
        QString filename = fileNames[fileIndex];
        int line = 100 + QRandomGenerator::global()->bounded(500);

        // Crear y agregar bloque básico
        MemoryMapTypes::BasicMemoryBlock newBlock;
        newBlock.address = address;
        newBlock.size = size;
        newBlock.type = "alloc";
        newBlock.state = "active";
        newBlock.filename = filename;
        newBlock.line = line;
        memoryBlocks.append(newBlock);

        // Registrar en contexto actual
        allocateInCurrentContext(address);
        normalAllocations++;

        sendMemoryEvent("alloc", address, size, filename, line);

        qDebug() << "🆕 NUEVA ASIGNACIÓN - Total activas:"
                 << std::count_if(memoryBlocks.begin(), memoryBlocks.end(),
                                  [](const MemoryMapTypes::BasicMemoryBlock& b) { return b.state == "active"; });
    }
    else if (actionType < 75 && !memoryBlocks.isEmpty()) // 🔴 15% de liberaciones
    {
        // Liberación - SOLO si hay suficientes bloques activos
        QVector<int> activeIndices;
        for (int i = 0; i < memoryBlocks.size(); ++i) {
            if (memoryBlocks[i].state == "active") {
                activeIndices.append(i);
            }
        }

        if (!activeIndices.isEmpty()) {
            int randomIndex = QRandomGenerator::global()->bounded(activeIndices.size());
            int blockIndex = activeIndices[randomIndex];
            auto &block = memoryBlocks[blockIndex];

            sendMemoryEvent("free", block.address, block.size, block.filename, block.line);
            block.state = "freed";
            totalFreed++;

            qDebug() << "🗑️  BLOQUE LIBERADO - Quedan activas:"
                     << std::count_if(memoryBlocks.begin(), memoryBlocks.end(),
                                      [](const MemoryMapTypes::BasicMemoryBlock& b) { return b.state == "active"; });
        }
    }
    else if (actionType < 85 && !memoryBlocks.isEmpty())
    {
        // 10%: Reasignación
        QVector<int> activeIndices;
        for (int i = 0; i < memoryBlocks.size(); ++i) {
            if (memoryBlocks[i].state == "active") {
                activeIndices.append(i);
            }
        }

        if (!activeIndices.isEmpty()) {
            int randomIndex = QRandomGenerator::global()->bounded(activeIndices.size());
            int blockIndex = activeIndices[randomIndex];
            auto &block = memoryBlocks[blockIndex];

            sendMemoryEvent("free", block.address, block.size, block.filename, block.line);
            // Nueva asignación con diferente tamaño
            quint64 newSize = block.size * 2;
            sendMemoryEvent("alloc", block.address, newSize, block.filename, block.line);
            block.size = newSize;
            // MANTENER como ACTIVO
        }
    }
    else
    {
        // 🔴 15%: CREAR LEAK DIRECTO
        if (!memoryBlocks.isEmpty()) {
            QVector<int> activeIndices;
            for (int i = 0; i < memoryBlocks.size(); ++i) {
                if (memoryBlocks[i].state == "active") {
                    activeIndices.append(i);
                }
            }

            if (!activeIndices.isEmpty()) {
                int randomIndex = QRandomGenerator::global()->bounded(activeIndices.size());
                int blockIndex = activeIndices[randomIndex];
                auto &block = memoryBlocks[blockIndex];

                // 🆕 CAMBIO: Usar "leak" en lugar de "leaked"
                block.state = "leak";
                forcedLeaksCreated++;

                // 🆕 ENVIAR EVENTO DE LEAK ESPECÍFICO
                sendLeakEvent(block.address, block.size, block.filename, block.line);

                qDebug() << "🔴🎯 LEAK DIRECTO CREADO - Addr: 0x"
                         << QString::number(block.address, 16)
                         << "Size:" << block.size << "bytes"
                         << "File:" << block.filename;
            }
        }
    }

    // También enviar snapshot completo periódicamente
    if (memoryMapCount % 5 == 0)
    {
        globalClient->sendBasicMemoryMap(memoryBlocks);

        int active = std::count_if(memoryBlocks.begin(), memoryBlocks.end(),
                                   [](const MemoryMapTypes::BasicMemoryBlock& b) { return b.state == "active"; });
        int leaked = std::count_if(memoryBlocks.begin(), memoryBlocks.end(),
                                   [](const MemoryMapTypes::BasicMemoryBlock& b) { return b.state == "leak"; }); // 🆕 CAMBIO: "leak"
        int freed = std::count_if(memoryBlocks.begin(), memoryBlocks.end(),
                                  [](const MemoryMapTypes::BasicMemoryBlock& b) { return b.state == "freed"; });

        qDebug() << "📦 Snapshot completo - Activas:" << active
                 << "Leaks:" << leaked << "Liberadas:" << freed;
    }

    qDebug() << "🗺️  [" << memoryMapCount << "] EVENTOS_MEMORIA -"
             << "Total bloques:" << memoryBlocks.size();
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
    qDebug() << "🧠 MEMORY PROFILER - PRUEBA COMPLETA CON LEAKS VISIBLES";
    qDebug() << "================================================";
    qDebug() << "🔴 CONFIGURACIÓN MEJORADA PARA LEAKS:";
    qDebug() << "   - Estado de leak: 'leak' (consistente con GUI)";
    qDebug() << "   - Eventos de leak específicos enviados";
    qDebug() << "   - 60% probabilidad de NUEVAS ASIGNACIONES";
    qDebug() << "   - SOLO 15% probabilidad de LIBERACIONES";
    qDebug() << "   - 60% de LEAKS al salir de contextos";
    qDebug() << "   - 15% de LEAKS DIRECTOS adicionales";
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
                             qDebug() << "✅ PRUEBA COMPLETADA - RESUMEN DE LEAKS";
                             qDebug() << "================================================";

                             int finalActive = std::count_if(memoryBlocks.begin(), memoryBlocks.end(),
                                                             [](const MemoryMapTypes::BasicMemoryBlock& b) { return b.state == "active"; });
                             int finalLeaked = std::count_if(memoryBlocks.begin(), memoryBlocks.end(),
                                                             [](const MemoryMapTypes::BasicMemoryBlock& b) { return b.state == "leak"; }); // 🆕 CAMBIO: "leak"
                             int finalFreed = std::count_if(memoryBlocks.begin(), memoryBlocks.end(),
                                                            [](const MemoryMapTypes::BasicMemoryBlock& b) { return b.state == "freed"; });

                             qDebug() << "📊 ESTADO FINAL DE BLOQUES:";
                             qDebug() << "   - ACTIVOS:    " << finalActive << "(estos son LEAKS)";
                             qDebug() << "   - MARCADOS:   " << finalLeaked << "(leaks explícitos)";
                             qDebug() << "   - LIBERADOS:  " << finalFreed;
                             qDebug() << "   - TOTAL:      " << memoryBlocks.size();
                             qDebug() << "";
                             qDebug() << "🔴 LEAKS GENERADOS:";
                             qDebug() << "   - LEAKS FORZADOS:    " << forcedLeaksCreated;
                             qDebug() << "   - EVENTOS LEAK ENVIADOS: " << leakEventsSent;
                             qDebug() << "   - CONTEXTOS:         " << currentContextId;
                             qDebug() << "   - DETECCIONES:       " << contextLeakCounter;
                             qDebug() << "";
                             qDebug() << "📈 ESTADÍSTICAS:";
                             qDebug() << "   - ASIGNACIONES:      " << normalAllocations;
                             qDebug() << "   - LIBERACIONES:      " << totalFreed;
                             qDebug() << "   - GENERAL_METRICS:   " << metricsCount;
                             qDebug() << "   - TIMELINE_POINT:    " << timelineCount;
                             qDebug() << "   - TOP_FILES:         " << topFilesCount;
                             qDebug() << "   - BASIC_MEMORY_MAP:  " << memoryMapCount;
                             qDebug() << "================================================";
                             qDebug() << "🎯 RESULTADO: " << (finalActive + finalLeaked) << "LEAKS VISIBLES EN GUI 🎯";

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
