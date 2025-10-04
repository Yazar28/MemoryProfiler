#include <QCoreApplication>
#include <QTimer>
#include <QThread>
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

    // Conectar señales
    QObject::connect(&client, &Client::connected, &app, &QCoreApplication::quit);
    QObject::connect(&client, &Client::disconnected, &app, &QCoreApplication::quit);

    // Conectar al servidor
    quint16 port = 8080;
    if (argc > 1)
        port = QString(argv[1]).toUShort();

    if (!client.connectToServer("localhost", port))
    {
        qDebug() << "No se pudo conectar al servidor. Saliendo...";
        return -1;
    }

    // Esperar para asegurar conexión
    QThread::msleep(100);

    // ENVIAR MÉTRICAS GENERALES
    qDebug() << "\n--- Enviando métricas generales ---";
    GeneralMetrics metrics;
    metrics.currentUsageMB = 45.7;
    metrics.activeAllocations = 1234;
    metrics.memoryLeaksMB = 2.3;
    metrics.maxMemoryMB = 67.8;
    metrics.totalAllocations = 56789;

    client.send("GENERAL_METRICS", metrics);

    // Programar desconexión
    QTimer::singleShot(2000, [&]()
                       {
                           client.disconnectFromServer();
                           QTimer::singleShot(500, &app, &QCoreApplication::quit);
                       });

    return app.exec();
}

