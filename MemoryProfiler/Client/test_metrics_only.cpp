//=============================== Que hace la prueba ===============================//
// prueba teorica para enviar structuras de datos al servidor
// Se probara la comunicación enviando datos de GeneralMetrics y TimelinePoint
// Se probara que se grafiquen correctamente en el GUI
// Se enviaran datos con diferentes patrones para probar la robustez del sistema
// se probara la maneraen que se envia la información
// Se probara la eficiecia y efectividad de las librerias ProfilerStructures.h y ServerClient.h
//===============================//
// librerías y archivos externos
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
    // Serializar cada campo de GeneralMetrics(prueba de serialización)
    stream << metrics.currentUsageMB    // representa el uso de memoria actual en MB(serialisado)
           << metrics.activeAllocations // representa el número de asignaciones activas(serialisado)
           << metrics.memoryLeaksMB     // representa la memoria total perdida en MB(serialisado)
           << metrics.maxMemoryMB       // representa el uso máximo de memoria registrado en MB(serialisado)
           << metrics.totalAllocations; // representa el número total de asignaciones realizadas(serialisado)
    return stream;                      // retornar el flujo modificado (stream)
}
// Sobrecarga del operador << para TimelinePoint
QDataStream &operator<<(QDataStream &stream, const TimelinePoint &point) // Sobrecarga del operador << para TimelinePoint( prueba de serialización)
{
    stream << point.timestamp << point.memoryMB; // Serializar los campos de TimelinePoint
    return stream;
}
int main(int argc, char *argv[]) // Función principal(prueba general)
{
    QCoreApplication app(argc, argv); // Crear la aplicación Qt(para enviar algo a qt tiene que haber una aplicación qt)

    Client client; // Crear instancia del cliente
    // Conectar al servidor
    quint16 port = 8080; // representa el puerto por defecto
    // Intentar conectar al servidor
    if (argc > 1)                           // caso si se proporciona un puerto como argumento
        port = QString(argv[1]).toUShort(); // convertir el argumento a número

    qDebug() << "Conectando al servidor en localhost:" << port;
    if (!client.connectToServer("localhost", port)) // caso no se pudo conectar al servidor
    {
        qDebug() << "No se pudo conectar al servidor. Saliendo...";
        return -1; // salir con error -1(-1 es un error general de la aplicación)
    }
    // Esperar para asegurar conexión
    QThread::msleep(200); // representa un hilo que duerme por 200 milisegundos(tiempo estamdar para conexiones locales)
    // Prueba completa: Enviar datos de GeneralMetrics y TimelinePoint con diferentes patrones
    qDebug() << "\n=== INICIANDO PRUEBA COMPLETA (GENERAL_METRICS + TIMELINE) ===";
    // Variables para simular diferentes patrones de uso de memoria
    int testCount = 0;         // representa el contador de pruebas
    double baseMemory = 100.0; // representa la memoria base inicial en MB
    // Timer para enviar datos cada segundo
    QTimer *timer = new QTimer(&app); // Crear un timer asociado a la aplicación
    /// Conectar la señal de timeout del timer (timer es un cronometro) a una lambda(lambda es una función anónima)
    /// prueba de envío periódico de datos
    /// funsiuna creando una función anónima que se ejecuta cada vez que el timer hace "timeout"
    /// esto prueba la parte de envío periódico de datosa una grafica
    QObject::connect(timer, &QTimer::timeout, [&]()
                     {
            testCount++; // Incrementar contador de pruebas
            qDebug() << "\n--- Enviando datos #" << testCount << "---"; // Mensaje de inicio de envío
            // === ENVIAR GENERAL_METRICS ===//
            GeneralMetrics metrics; //representa las métricas generales
            metrics.currentUsageMB = baseMemory + (QRandomGenerator::global()->generate() % 50);// asignar uso de memoria actual con algo de aleatoriedad
            metrics.activeAllocations = 800 + (QRandomGenerator::global()->generate() % 500);// asignar número de asignaciones activas con algo de aleatoriedad
            metrics.memoryLeaksMB = 2.0 + (QRandomGenerator::global()->generate() % 15);// asignar memoria perdida con algo de aleatoriedad
            metrics.maxMemoryMB = 150.0 + (QRandomGenerator::global()->generate() % 100);// asignar uso máximo de memoria con algo de aleatoriedad
            metrics.totalAllocations = 30000 + (QRandomGenerator::global()->generate() % 20000);// asignar número total de asignaciones con algo de aleatoriedad
            // Enviar los datos al servidor usando el cliente
            client.send("GENERAL_METRICS", metrics);
            // Mensaje de éxito con detalles
            qDebug() << "Enviado GENERAL_METRICS - Current:" << metrics.currentUsageMB << "MB" //representa el uso de memoria actual en MB
                    << "Active:" << metrics.activeAllocations // representa el número de asignaciones activas
                    << "Leaks:" << metrics.memoryLeaksMB << "MB" // representa la memoria total perdida en MB
                    << "Max:" << metrics.maxMemoryMB << "MB" //representa el uso máximo de memoria registrado en MB
                    << "Total:" << metrics.totalAllocations; // representa el número total de asignaciones realizadas
            // === ENVIAR TIMELINE_POINT ===
            TimelinePoint timelinePoint; // representa un punto en la línea de tiempo
            timelinePoint.timestamp = QDateTime::currentMSecsSinceEpoch(); // asignar la marca de tiempo actual
            // Simular diferentes patrones de uso de memoria a lo largo del tiempo
            if (testCount <= 10)  // caso en la fase 1 cuando el contador de pruebas es menor o igual a 10
            {
                // Fase 1: Crecimiento constante
                timelinePoint.memoryMB = 80.0 + (testCount * 8) + (QRandomGenerator::global()->generate() % 20); // crecimiento lineal con algo de aleatoriedad
            } 
            else if (testCount <= 25) // caso en la fase 2 cuando el contador de pruebas es menor o igual a 25
            {
                // Fase 2: Oscilación con picos
                double oscillation = 30 * qSin(testCount * 0.5);
                timelinePoint.memoryMB = 150.0 + oscillation + (QRandomGenerator::global()->generate() % 25);
            } 
            else if (testCount <= 40) // caso en la fase 3 cuando el contador de pruebas es menor o igual a 40
            {
                // Fase 3: Subida abrupta y luego descenso
                if (testCount <= 30) // caso en la fase 3.1 cuando el contador de pruebas es menor o igual a 30
                {
                    timelinePoint.memoryMB = 120.0 + ((testCount - 25) * 15) + (QRandomGenerator::global()->generate() % 30); // subida abrupta
                } 
                else // caso en la fase 3.2 cuando el contador de pruebas es mayor a 30
                {
                    timelinePoint.memoryMB = 270.0 - ((testCount - 30) * 12) + (QRandomGenerator::global()->generate() % 20);// descenso
                }
            } 
            else if (testCount <= 60) 
            {
                // Fase 4: Patrón estable con pequeñas variaciones
                timelinePoint.memoryMB = 100.0 + (QRandomGenerator::global()->generate() % 40);// uso estable con algo de aleatoriedad
            } 
            else // caso en la fase 5 cuando el contador de pruebas es mayor a 60
            {
                // Fase 5: Crecimiento final
                timelinePoint.memoryMB = 80.0 + ((testCount - 60) * 5) + (QRandomGenerator::global()->generate() % 15);// crecimiento lento con algo de aleatoriedad
            }

            client.send("TIMELINE_POINT", timelinePoint); // Enviar el punto de la línea de tiempo al servidor
            // Mensaje de éxito con detalles
            qDebug() << "Enviado TIMELINE_POINT - Time:" << timelinePoint.timestamp //representa la marca de tiempo en milisegundos desde epoch
                    << "Memory:" << timelinePoint.memoryMB << "MB"; // representa el uso de memoria en MB en este punto de tiempo
            // Actualizar memoria base para siguiente iteración
            baseMemory = timelinePoint.memoryMB * 0.1 + baseMemory * 0.9;//representa una media móvil para suavizar cambios bruscos
            // Detener después de 80 envíos (prueba extensa)
            if (testCount >= 80)  // caso si el contador de pruebas es mayor o igual a 80
            {
                qDebug() << "\n=== PRUEBA COMPLETADA ==="; // Mensaje de finalización de la prueba
                qDebug() << "Total de envíos:" << testCount; // Mensaje con el total de envíos realizados
                qDebug() << "Tiempo total de prueba:" << testCount << "segundos";// Mensaje con el tiempo total de la prueba
                timer->stop(); // Detener el timer
                client.disconnectFromServer(); // Desconectar del servidor
                QTimer::singleShot(1000, &app, &QCoreApplication::quit);// Salir de la aplicación después de 1 segundo
            } }); // Fin de la conexión de la señal timeout(prueba de envío periódico de datos)

    // Iniciar el timer cada 1 segundo para mejor resolución temporal
    timer->start(1000); // Iniciar el timer con intervalo de 1000 ms (1 segundo)

    return app.exec(); // ejecutar el bucle de eventos de la aplicación
}
