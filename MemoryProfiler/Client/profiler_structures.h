#ifndef PROFILER_STRUCTURES_H
#define PROFILER_STRUCTURES_H
// librerías y archivos externos
#include <QString>
#include <QVector>
#include <QDateTime>
//================================
// Estructuras para Vista General
//================================
struct GeneralMetrics // Métricas generales
{
    double currentUsageMB = 0.0;   // represneta el uso de memoria actual en MB
    quint64 activeAllocations = 0; // número de asignaciones activas
    double memoryLeaksMB = 0.0;    // memoria total perdida en MB
    double maxMemoryMB = 0.0;      // uso máximo de memoria registrado en MB
    quint64 totalAllocations = 0;  // número total de asignaciones realizadas
};
struct TimelinePoint // Punto en la línea de tiempo
{
    qint64 timestamp; // reprecenta la marca de tiempo en milisegundos desde epoch
    double memoryMB;  // uso de memoria en MB en este punto de tiempo
};
struct TopFile // Archivo con más asignaciones
{
    QString filename;        // representa el nombre del archivo
    quint64 allocations = 0; // número de asignaciones realizadas en este archivo
    double memoryMB = 0.0;   // memoria total asignada por este archivo en MB
};
// ================================
// Estructuras para Mapa de Memoria
// ================================
struct MemoryBlock
{
    quint64 address = 0;  // repreneta la dirección de memoria
    quint64 size = 0;     // representa el tamaño del bloque de memoria en bytes
    QString type;         // representa el tipo de asignación, por ejemplo, "new", "malloc", etc.
    QString state;        // representa el estado del bloque de memoria, por ejemplo, "active", "freed", etc.
    QString filename;     // representa el nombre del archivo donde se realizó la asignación
    int line = 0;         // representa la línea en el archivo donde se realizó la asignación
    qint64 timestamp = 0; // marca de tiempo de la asignación en milisegundos desde epoch
};
// ================================
// Estructuras para Asignación por Archivo
// ================================
struct FileAllocation // Asignación de memoria por archivo
{
    QString filename;            // representa el renombre del archivo
    quint64 allocationCount = 0; // representa el número de asignaciones realizadas en este archivo
    double memoryMB = 0.0;       // representa la memoria total asignada por este archivo en MB
};
// ================================
// Estructuras para Memory Leaks
// ================================
struct LeakSummary // Resumen de pérdidas de memoria
{
    double totalLeakedMB = 0.0;   // Memoria total perdida en MB
    double biggestLeakMB = 0.0;   // Memoria de la mayor pérdida en MB
    QString biggestLeakLocation;  // "archivo.cpp:123"
    QString mostFrequentLeakFile; // Archivo con más pérdidas
    double leakRate = 0.0;        // Porcentaje
};
struct LeakByFile // Pérdidas de memoria por archivo
{
    QString filename;      // representa el nombre del archivo
    double leakedMB = 0.0; // representa la memoria perdida en MB por este archivo
    quint64 leakCount = 0; // representa el número de pérdidas de memoria en este archivo
};
struct LeakTimelinePoint // Punto en la línea de tiempo de pérdidas de memoria
{
    qint64 timestamp;  // representa la marca de tiempo en milisegundos desde epoch
    double leakedMB;   // representa la memoria perdida en MB en este punto de tiempo
    quint64 leakCount; // representa el número de pérdidas de memoria en este punto de tiempo
};
// ================================
// Estructuras de Comunicación
// ================================
struct ProfilerData // Estructura principal que contiene todos los datos del profiler
{
    // Vista General
    GeneralMetrics metrics;          // representa las métricas generales
    QVector<TimelinePoint> timeline; // Línea de tiempo de uso de memoria
    QVector<TopFile> topFiles;       // Top archivos con más asignaciones

    // Mapa de Memoria
    QVector<MemoryBlock> memoryMap; // Mapa de memoria

    // Asignación por Archivo
    QVector<FileAllocation> fileAllocations; // Asignación de memoria por archivo

    // Memory Leaks
    LeakSummary leakSummary;                 // Resumen de pérdidas de memoria
    QVector<LeakByFile> leaksByFile;         // Pérdidas de memoria por archivo
    QVector<LeakTimelinePoint> leakTimeline; // Línea de tiempo de pérdidas de memoria
};
#endif // PROFILER_STRUCTURES_H
