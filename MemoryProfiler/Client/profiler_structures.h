#ifndef PROFILER_STRUCTURES_H
#define PROFILER_STRUCTURES_H

#include <QString>
#include <QVector>
#include <QDateTime>

// ================================
// Estructuras para Vista General
// ================================

struct GeneralMetrics
{
    double currentUsageMB = 0.0;
    quint64 activeAllocations = 0;
    double memoryLeaksMB = 0.0;
    double maxMemoryMB = 0.0;
    quint64 totalAllocations = 0;
};

struct TimelinePoint
{
    qint64 timestamp;
    double memoryMB;
};

struct TopFile
{
    QString filename;
    quint64 allocations = 0;
    double memoryMB = 0.0;
};

// ================================
// Estructuras para Mapa de Memoria
// ================================

struct MemoryBlock
{
    quint64 address = 0;
    quint64 size = 0;
    QString type;
    QString state; // "Allocated", "Freed", "Leaked"
    QString filename;
    int line = 0;
    qint64 timestamp = 0;
};

// ================================
// Estructuras para Asignación por Archivo
// ================================

struct FileAllocation
{
    QString filename;
    quint64 allocationCount = 0;
    double memoryMB = 0.0;
};

// ================================
// Estructuras para Memory Leaks
// ================================

struct LeakSummary
{
    double totalLeakedMB = 0.0;
    double biggestLeakMB = 0.0;
    QString biggestLeakLocation; // "archivo.cpp:123"
    QString mostFrequentLeakFile;
    double leakRate = 0.0; // Porcentaje
};

struct LeakByFile
{
    QString filename;
    double leakedMB = 0.0;
    quint64 leakCount = 0;
};

struct LeakTimelinePoint
{
    qint64 timestamp;
    double leakedMB;
    quint64 leakCount;
};

// ================================
// Estructuras de Comunicación
// ================================

struct ProfilerData
{
    // Vista General
    GeneralMetrics metrics;
    QVector<TimelinePoint> timeline;
    QVector<TopFile> topFiles;

    // Mapa de Memoria
    QVector<MemoryBlock> memoryMap;

    // Asignación por Archivo
    QVector<FileAllocation> fileAllocations;

    // Memory Leaks
    LeakSummary leakSummary;
    QVector<LeakByFile> leaksByFile;
    QVector<LeakTimelinePoint> leakTimeline;
};

#endif // PROFILER_STRUCTURES_H
