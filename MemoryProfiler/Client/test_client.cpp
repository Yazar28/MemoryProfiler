#include <QCoreApplication>
#include <QTimer>
#include <QThread>
#include "ServerClient.h"

// Estructuras de ejemplo para el memory profiler
struct MemoryAllocation
{
    quint64 address;
    quint64 size;
    QString filename;
    int line;
    qint64 timestamp;
};

struct MemoryMetrics
{
    double currentUsageMB;
    quint64 activeAllocations;
    double memoryLeaksMB;
    double peakUsageMB;
    quint64 totalAllocations;
};

struct MemoryLeak
{
    quint64 address;
    quint64 size;
    QString filename;
    int line;
};

// Sobrecarga de operadores para serialización
QDataStream &operator<<(QDataStream &stream, const MemoryAllocation &alloc)
{
    stream << alloc.address << alloc.size << alloc.filename << alloc.line << alloc.timestamp;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const MemoryMetrics &metrics)
{
    stream << metrics.currentUsageMB << metrics.activeAllocations
           << metrics.memoryLeaksMB << metrics.peakUsageMB << metrics.totalAllocations;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const MemoryLeak &leak)
{
    stream << leak.address << leak.size << leak.filename << leak.line;
    return stream;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    Client client;

    // Conectar señales usando la sintaxis tradicional de Qt
    QObject::connect(&client, SIGNAL(connected()), &app, SLOT(quit()));
    QObject::connect(&client, SIGNAL(disconnected()), &app, SLOT(quit()));

    // Conectar al servidor (puerto personalizable)
    quint16 port = 8080;
    if (argc > 1)
        port = QString(argv[1]).toUShort();

    if (!client.connectToServer("localhost", port))
    {
        qDebug() << "No se pudo conectar al servidor. Saliendo...";
        return -1;
    }

    // Esperar un poco para asegurar conexión
    QThread::msleep(100);

    // PRUEBA 1: Enviar métricas de memoria
    qDebug() << "\n--- Enviando métricas de memoria ---";
    MemoryMetrics metrics;
    metrics.currentUsageMB = 45.7;
    metrics.activeAllocations = 1234;
    metrics.memoryLeaksMB = 2.3;
    metrics.peakUsageMB = 67.8;
    metrics.totalAllocations = 56789;

    client.send("MEMORY_METRICS", metrics);

    // PRUEBA 2: Enviar asignación de memoria
    qDebug() << "\n--- Enviando asignación de memoria ---";
    MemoryAllocation alloc;
    alloc.address = 0x12345678;
    alloc.size = 1024;
    alloc.filename = "main.cpp";
    alloc.line = 42;
    alloc.timestamp = QDateTime::currentMSecsSinceEpoch();

    client.send("MEMORY_ALLOC", alloc);

    // PRUEBA 3: Enviar memory leak
    qDebug() << "\n--- Enviando memory leak ---";
    MemoryLeak leak;
    leak.address = 0x87654321;
    leak.size = 512;
    leak.filename = "utils.cpp";
    leak.line = 15;

    client.send("MEMORY_LEAK", leak);

    // PRUEBA 4: Enviar datos simples
    qDebug() << "\n--- Enviando datos simples ---";
    client.send("STRING_TEST", QString("Hola desde el cliente!"));
    client.send("INT_TEST", 42);
    client.send("DOUBLE_TEST", 3.14159);

    // Programar desconexión y salida
    QTimer::singleShot(2000, [&]()
                       {
        client.disconnectFromServer();
        QTimer::singleShot(500, &app, &QCoreApplication::quit); });

    return app.exec();
}