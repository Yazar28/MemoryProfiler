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

    qDebug() << "\n=== INICIANDO PRUEBA SOLO GENERAL_METRICS ===";

    int testCount = 0;

    // Timer para enviar GENERAL_METRICS cada 2 segundos
    QTimer *timer = new QTimer(&app);
    QObject::connect(timer, &QTimer::timeout, [&]()
                     {
        testCount++;
        
        qDebug() << "\n--- Enviando GENERAL_METRICS #" << testCount << "---";
        
        GeneralMetrics metrics;
        metrics.currentUsageMB = 100.0 + (QRandomGenerator::global()->generate() % 100);
        metrics.activeAllocations = 800 + (QRandomGenerator::global()->generate() % 500);
        metrics.memoryLeaksMB = 2.0 + (QRandomGenerator::global()->generate() % 15);
        metrics.maxMemoryMB = 150.0 + (QRandomGenerator::global()->generate() % 100);
        metrics.totalAllocations = 30000 + (QRandomGenerator::global()->generate() % 20000);
        
        client.send("GENERAL_METRICS", metrics);
        
        qDebug() << "Enviado - Current:" << metrics.currentUsageMB << "MB"
                 << "Active:" << metrics.activeAllocations
                 << "Leaks:" << metrics.memoryLeaksMB << "MB"
                 << "Max:" << metrics.maxMemoryMB << "MB"
                 << "Total:" << metrics.totalAllocations;

        // Detener después de 5 envíos
        if (testCount >= 5) {
            qDebug() << "\n=== PRUEBA COMPLETADA ===";
            timer->stop();
            client.disconnectFromServer();
            QTimer::singleShot(1000, &app, &QCoreApplication::quit);
        } });

    // Iniciar el timer cada 2 segundos
    timer->start(2000);

    return app.exec();
}
