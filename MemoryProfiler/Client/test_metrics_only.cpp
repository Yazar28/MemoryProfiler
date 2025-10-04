#include <QCoreApplication>
#include <QTimer>
#include <QThread>
#include <QDebug>
#include <QRandomGenerator>
#include "ServerClient.h"
#include "profiler_structures.h"

// Sobrecarga del operador << para GeneralMetrics
QDataStream &operator<<(QDataStream &stream, const GeneralMetrics &metrics)
{
    stream << metrics.currentUsageMB
           << metrics.activeAllocations
           << metrics.memoryLeaksMB
           << metrics.maxMemoryMB
           << metrics.totalAllocations;
    return stream;
}

// Sobrecarga del operador << para TimelinePoint
QDataStream &operator<<(QDataStream &stream, const TimelinePoint &point)
{
    stream << point.timestamp << point.memoryMB;
    return stream;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    Client client;

    // Conectar al servidor
    quint16 port = 8080;
    if (argc > 1)
        port = QString(argv[1]).toUShort();

    qDebug() << "Conectando al servidor en localhost:" << port;

    if (!client.connectToServer("localhost", port))
    {
        qDebug() << "No se pudo conectar al servidor. Saliendo...";
        return -1;
    }

    // Esperar para asegurar conexión
    QThread::msleep(200);

    qDebug() << "\n=== INICIANDO PRUEBA COMPLETA (GENERAL_METRICS + TIMELINE) ===";

    int testCount = 0;
    double baseMemory = 100.0; // Memoria base para simular tendencia

    // Timer para enviar datos cada segundo
    QTimer *timer = new QTimer(&app);
    QObject::connect(timer, &QTimer::timeout, [&]()
                     {
                         testCount++;

                         qDebug() << "\n--- Enviando datos #" << testCount << "---";

                         // === ENVIAR GENERAL_METRICS ===
                         GeneralMetrics metrics;
                         metrics.currentUsageMB = baseMemory + (QRandomGenerator::global()->generate() % 50);
                         metrics.activeAllocations = 800 + (QRandomGenerator::global()->generate() % 500);
                         metrics.memoryLeaksMB = 2.0 + (QRandomGenerator::global()->generate() % 15);
                         metrics.maxMemoryMB = 150.0 + (QRandomGenerator::global()->generate() % 100);
                         metrics.totalAllocations = 30000 + (QRandomGenerator::global()->generate() % 20000);

                         client.send("GENERAL_METRICS", metrics);

                         qDebug() << "Enviado GENERAL_METRICS - Current:" << metrics.currentUsageMB << "MB"
                                  << "Active:" << metrics.activeAllocations
                                  << "Leaks:" << metrics.memoryLeaksMB << "MB"
                                  << "Max:" << metrics.maxMemoryMB << "MB"
                                  << "Total:" << metrics.totalAllocations;

                         // === ENVIAR TIMELINE_POINT ===
                         TimelinePoint timelinePoint;
                         timelinePoint.timestamp = QDateTime::currentMSecsSinceEpoch();

                         // Simular diferentes patrones de uso de memoria a lo largo del tiempo
                         if (testCount <= 10) {
                             // Fase 1: Crecimiento constante
                             timelinePoint.memoryMB = 80.0 + (testCount * 8) + (QRandomGenerator::global()->generate() % 20);
                         } else if (testCount <= 25) {
                             // Fase 2: Oscilación con picos
                             double oscillation = 30 * qSin(testCount * 0.5);
                             timelinePoint.memoryMB = 150.0 + oscillation + (QRandomGenerator::global()->generate() % 25);
                         } else if (testCount <= 40) {
                             // Fase 3: Subida abrupta y luego descenso
                             if (testCount <= 30) {
                                 timelinePoint.memoryMB = 120.0 + ((testCount - 25) * 15) + (QRandomGenerator::global()->generate() % 30);
                             } else {
                                 timelinePoint.memoryMB = 270.0 - ((testCount - 30) * 12) + (QRandomGenerator::global()->generate() % 20);
                             }
                         } else if (testCount <= 60) {
                             // Fase 4: Patrón estable con pequeñas variaciones
                             timelinePoint.memoryMB = 100.0 + (QRandomGenerator::global()->generate() % 40);
                         } else {
                             // Fase 5: Crecimiento final
                             timelinePoint.memoryMB = 80.0 + ((testCount - 60) * 5) + (QRandomGenerator::global()->generate() % 15);
                         }

                         client.send("TIMELINE_POINT", timelinePoint);

                         qDebug() << "Enviado TIMELINE_POINT - Time:" << timelinePoint.timestamp
                                  << "Memory:" << timelinePoint.memoryMB << "MB";

                         // Actualizar memoria base para siguiente iteración
                         baseMemory = timelinePoint.memoryMB * 0.1 + baseMemory * 0.9;

                         // Detener después de 80 envíos (prueba extensa)
                         if (testCount >= 80) {
                             qDebug() << "\n=== PRUEBA COMPLETADA ===";
                             qDebug() << "Total de envíos:" << testCount;
                             qDebug() << "Tiempo total de prueba:" << testCount << "segundos";
                             timer->stop();
                             client.disconnectFromServer();
                             QTimer::singleShot(1000, &app, &QCoreApplication::quit);
                         } });

    // Iniciar el timer cada 1 segundo para mejor resolución temporal
    timer->start(1000);

    return app.exec();
}
