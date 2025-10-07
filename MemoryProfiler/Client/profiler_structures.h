#ifndef PROFILER_STRUCTURES_H
#define PROFILER_STRUCTURES_H

// librerías y archivos externos
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QDataStream>

// ================================
// Estructuras para Vista General
// ================================
struct MemoryEvent
{
    quint64 address = 0;
    quint64 size = 0;
    QString type;
    QString event_type; // "alloc", "free", "realloc"
    QString filename;
    int line = 0;
    qint64 timestamp = 0;
    QString stack_trace; // Opcional para debugging
};


struct GeneralMetrics // Métricas generales
{
    double currentUsageMB = 0.0;   // representa el uso de memoria actual en MB
    quint64 activeAllocations = 0; // número de asignaciones activas
    double memoryLeaksMB = 0.0;    // memoria total perdida en MB
    double maxMemoryMB = 0.0;      // uso máximo de memoria registrado en MB
    quint64 totalAllocations = 0;  // número total de asignaciones realizadas
};

struct TimelinePoint // Punto en la línea de tiempo
{
    qint64 timestamp; // representa la marca de tiempo en milisegundos desde epoch
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
    quint64 address = 0;  // representa la dirección de memoria
    quint64 size = 0;     // representa el tamaño del bloque de memoria en bytes
    QString type;         // representa el tipo de asignación, por ejemplo, "new", "malloc", etc.
    QString state;        // representa el estado del bloque de memoria, por ejemplo, "active", "freed", etc.
    QString filename;     // representa el nombre del archivo donde se realizó la asignación
    int line = 0;         // representa la línea en el archivo donde se realizó la asignación
    qint64 timestamp = 0; // marca de tiempo de la asignación en milisegundos desde epoch
};

// ================================
// Estructuras Flexibles para Memory Map
// ================================

namespace MemoryMapTypes
{

// 🎯 Versión básica para visualización (más eficiente)
struct BasicMemoryBlock
{
    quint64 address;
    quint64 size;
    QString type;
    QString state;
    QString filename;
    int line;
};

// 🎯 Versión extendida para análisis detallado
struct DetailedMemoryBlock : public MemoryBlock
{
    quint64 allocationId = 0;
    QString functionName;
    QString callStack;
    double memoryMB = 0.0;

    DetailedMemoryBlock() = default;
    DetailedMemoryBlock(const MemoryBlock &block) : MemoryBlock(block)
    {
        memoryMB = size / (1024.0 * 1024.0);
    }
};

// 🎯 Solo estadísticas (muy liviano)
struct MemoryStats
{
    quint64 totalBlocks = 0;
    quint64 activeBlocks = 0;
    quint64 freedBlocks = 0;
    quint64 leakedBlocks = 0;
    double totalMemoryMB = 0.0;
    double activeMemoryMB = 0.0;
    double leakedMemoryMB = 0.0;
    qint64 snapshotTime = 0;
};

} // namespace MemoryMapTypes


// ================================
// Estructuras para Asignación por Archivo
// ================================

struct FileAllocation // Asignación de memoria por archivo
{
    QString filename;            // representa el nombre del archivo
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

// ================================
// Serializaciones
// ================================

// Sobrecarga del operador << para GeneralMetrics
inline QDataStream &operator<<(QDataStream &stream, const GeneralMetrics &metrics)
{
    stream << metrics.currentUsageMB
           << metrics.activeAllocations
           << metrics.memoryLeaksMB
           << metrics.maxMemoryMB
           << metrics.totalAllocations;
    return stream;
}

// Sobrecarga del operador >> para GeneralMetrics
inline QDataStream &operator>>(QDataStream &stream, GeneralMetrics &metrics)
{
    stream >> metrics.currentUsageMB >> metrics.activeAllocations >> metrics.memoryLeaksMB >> metrics.maxMemoryMB >> metrics.totalAllocations;
    return stream;
}

// Sobrecarga del operador << para TimelinePoint
inline QDataStream &operator<<(QDataStream &stream, const TimelinePoint &point)
{
    stream << point.timestamp << point.memoryMB;
    return stream;
}

// Sobrecarga del operador >> para TimelinePoint
inline QDataStream &operator>>(QDataStream &stream, TimelinePoint &point)
{
    stream >> point.timestamp >> point.memoryMB;
    return stream;
}

// Sobrecarga del operador << para TopFile
inline QDataStream &operator<<(QDataStream &stream, const TopFile &topFile)
{
    stream << topFile.filename << topFile.allocations << topFile.memoryMB;
    return stream;
}

// Sobrecarga del operador >> para TopFile
inline QDataStream &operator>>(QDataStream &stream, TopFile &topFile)
{
    stream >> topFile.filename >> topFile.allocations >> topFile.memoryMB;
    return stream;
}

// Sobrecarga del operador << para QVector<TopFile>
inline QDataStream &operator<<(QDataStream &stream, const QVector<TopFile> &topFiles)
{
    stream << quint32(topFiles.size());
    for (const TopFile &topFile : topFiles)
        stream << topFile;
    return stream;
}

// Sobrecarga del operador >> para QVector<TopFile>
inline QDataStream &operator>>(QDataStream &stream, QVector<TopFile> &topFiles)
{
    quint32 size;
    stream >> size;
    topFiles.resize(size);
    for (quint32 i = 0; i < size; ++i)
        stream >> topFiles[i];
    return stream;
}

// Sobrecarga del operador << para MemoryMapTypes::BasicMemoryBlock
inline QDataStream &operator<<(QDataStream &stream, const MemoryMapTypes::BasicMemoryBlock &block)
{
    stream << block.address
           << block.size
           << block.type
           << block.state
           << block.filename
           << quint32(block.line); // Convertir int a quint32 para serialización
    return stream;
}

// Sobrecarga del operador >> para MemoryMapTypes::BasicMemoryBlock
inline QDataStream &operator>>(QDataStream &stream, MemoryMapTypes::BasicMemoryBlock &block)
{
    quint32 line; // Variable temporal para la línea
    stream >> block.address >> block.size >> block.type >> block.state >> block.filename >> line;
    block.line = line; // Asignar de quint32 a int
    return stream;
}

// Sobrecarga del operador << para QVector<MemoryMapTypes::BasicMemoryBlock>
inline QDataStream &operator<<(QDataStream &stream, const QVector<MemoryMapTypes::BasicMemoryBlock> &blocks)
{
    stream << quint32(blocks.size());
    for (const MemoryMapTypes::BasicMemoryBlock &block : blocks)
        stream << block;
    return stream;
}

// Sobrecarga del operador >> para QVector<MemoryMapTypes::BasicMemoryBlock>
inline QDataStream &operator>>(QDataStream &stream, QVector<MemoryMapTypes::BasicMemoryBlock> &blocks)
{
    quint32 size;
    stream >> size;
    blocks.resize(size);
    for (quint32 i = 0; i < size; ++i)
        stream >> blocks[i];
    return stream;
}

// Sobrecarga del operador << para MemoryMapTypes::MemoryStats
inline QDataStream &operator<<(QDataStream &stream, const MemoryMapTypes::MemoryStats &stats)
{
    stream << stats.totalBlocks
           << stats.activeBlocks
           << stats.freedBlocks
           << stats.leakedBlocks
           << stats.totalMemoryMB
           << stats.activeMemoryMB
           << stats.leakedMemoryMB
           << stats.snapshotTime;
    return stream;
}

// Sobrecarga del operador >> para MemoryMapTypes::MemoryStats
inline QDataStream &operator>>(QDataStream &stream, MemoryMapTypes::MemoryStats &stats)
{
    stream >> stats.totalBlocks >> stats.activeBlocks >> stats.freedBlocks >> stats.leakedBlocks >> stats.totalMemoryMB >> stats.activeMemoryMB >> stats.leakedMemoryMB >> stats.snapshotTime;
    return stream;
}

// ================================
// Serialización para MemoryEvent
// ================================

// Sobrecarga del operador << para MemoryEvent
inline QDataStream &operator<<(QDataStream &stream, const MemoryEvent &event)
{
    stream << event.address
           << event.size
           << event.type
           << event.event_type
           << event.filename
           << quint32(event.line)
           << event.timestamp
           << event.stack_trace;
    return stream;
}

// Sobrecarga del operador >> para MemoryEvent
inline QDataStream &operator>>(QDataStream &stream, MemoryEvent &event)
{
    quint32 line;
    stream >> event.address
        >> event.size
        >> event.type
        >> event.event_type
        >> event.filename
        >> line
        >> event.timestamp
        >> event.stack_trace;
    event.line = line;
    return stream;
}

// Sobrecarga del operador << para QVector<MemoryEvent>
inline QDataStream &operator<<(QDataStream &stream, const QVector<MemoryEvent> &events)
{
    stream << quint32(events.size());
    for (const MemoryEvent &event : events)
        stream << event;
    return stream;
}

// Sobrecarga del operador >> para QVector<MemoryEvent>
inline QDataStream &operator>>(QDataStream &stream, QVector<MemoryEvent> &events)
{
    quint32 size;
    stream >> size;
    events.resize(size);
    for (quint32 i = 0; i < size; ++i)
        stream >> events[i];
    return stream;
}

#endif // PROFILER_STRUCTURES_H
