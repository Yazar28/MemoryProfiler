#include "ListenLogic.h"
#include <QChart>
#include <QBarSeries>
#include <QBarSet>
#include <QLineSeries>
#include <QPieSeries>
#include "profiler_structures.h"

void ListenLogic::processData(const QString &keyword, const QByteArray &data)
{
    qDebug() << "=== PROCESANDO DATOS ===";
    qDebug() << "Keyword:" << keyword;
    qDebug() << "Tamaño datos:" << data.size() << "bytes";

    // Procesar datos binarios (estructuras)
    if (keyword == "GENERAL_METRICS")
    {
        handleGeneralMetrics(data);
        return;
    }
    else if (keyword == "TIMELINE_POINT")
    {
        handleTimelinePoint(data);
        return;
    }
    else if (keyword == "TOP_FILE")
    {
        handleTopFile(data);
        return;
    }

    // Para otros keywords, procesar como string (formato antiguo)
    QString dataStr = QString::fromUtf8(data);
    QStringList parts = dataStr.split('|');

    if (keyword == "LIVE_UPDATE")
    {
        handleLiveUpdate(parts);
    }
    else if (keyword == "MEMORY_MAP")
    {
        handleMemoryMap(parts);
    }
    else if (keyword == "FILE_ALLOCATIONS")
    {
        handleFileAllocations(parts);
    }
    else if (keyword == "LEAK_REPORT")
    {
        handleLeakReport(parts);
    }
}
void ListenLogic::handleLiveUpdate(const QStringList &parts)
{
    if (parts.size() < 2)
        return;

    if (parts[0] == "ALLOC" && parts.size() >= 6)
    {
        quint64 addr = parts[1].toULongLong();
        quint64 size = parts[2].toULongLong();
        QString file = parts[3];
        int line = parts[4].toInt();
        QString type = parts[5];

        qDebug() << "[LIVE] ALLOC addr:" << formatAddress(addr)
                 << "size:" << size << "file:" << file << "line:" << line << "type:" << type;
    }
    if (parts[0] == "FREE" && parts.size() >= 2)
    {
        quint64 addr = parts[1].toULongLong();
        qDebug() << "[LIVE] FREE addr:" << formatAddress(addr);
    }
}


QDataStream &operator>>(QDataStream &stream, GeneralMetrics &metrics)
{
    stream >> metrics.currentUsageMB
        >> metrics.activeAllocations
        >> metrics.memoryLeaksMB
        >> metrics.maxMemoryMB
        >> metrics.totalAllocations;
    return stream;
}

void ListenLogic::handleGeneralMetrics(const QByteArray &data)
{
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);

    GeneralMetrics metrics;
    stream >> metrics;

    if (stream.status() != QDataStream::Ok) {
        qDebug() << "✗ Error: No se pudo deserializar GeneralMetrics";
        qDebug() << "Bytes recibidos:" << data.size();
        qDebug() << "Stream status:" << stream.status();
        return;
    }

    qDebug() << "✓ Métricas deserializadas CORRECTAMENTE";
    qDebug() << "  Current:" << metrics.currentUsageMB << "MB";
    qDebug() << "  Active:" << metrics.activeAllocations;
    qDebug() << "  Leaks:" << metrics.memoryLeaksMB << "MB";
    qDebug() << "  Max:" << metrics.maxMemoryMB << "MB";
    qDebug() << "  Total:" << metrics.totalAllocations;

    emit generalMetricsUpdated(metrics);
}

// Luego en ListenLogic.cpp
void ListenLogic::handleTimelinePoint(const QByteArray &data)
{
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);

    TimelinePoint point;
    stream >> point.timestamp >> point.memoryMB;

    if (stream.status() == QDataStream::Ok) {
        qDebug() << "✓ TimelinePoint recibido - Time:" << point.timestamp << "Memory:" << point.memoryMB << "MB";
        // Aquí puedes emitir una señal para actualizar la timeline si la tienes
    }
}

void ListenLogic::handleTopFile(const QByteArray &data)
{
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);

    TopFile file;
    stream >> file.filename >> file.allocations >> file.memoryMB;

    if (stream.status() == QDataStream::Ok) {
        qDebug() << "✓ TopFile recibido - File:" << file.filename << "Allocs:" << file.allocations << "Memory:" << file.memoryMB << "MB";
        // Aquí puedes emitir una señal para actualizar la tabla de top files
    }
}



void ListenLogic::handleMemoryMap(const QStringList &parts)
{
    if (parts.size() < 2 || parts[0] != "MEMORY_MAP_START")
        return;

    int blockCount = parts[1].toInt();
    qDebug() << "[MEM_MAP] Starting with" << blockCount << "blocks";

    int index = 2;
    for (int i = 0; i < blockCount && index + 5 <= parts.size(); i++)
    {
        if (parts[index] == "BLOCK")
        {
            quint64 addr = parts[index + 1].toULongLong();
            quint64 size = parts[index + 2].toULongLong();
            QString type = parts[index + 3];
            QString file = parts[index + 4];
            int line = parts[index + 5].toInt();

            qDebug() << "[BLOCK] addr:" << formatAddress(addr)
                     << "size:" << size << "type:" << type << "file:" << file << "line:" << line;
            index += 6;
        }
    }
}

void ListenLogic::handleFileAllocations(const QStringList &parts)
{
    if (parts.size() < 2 || parts[0] != "FILE_SUMMARY_START")
        return;

    int fileCount = parts[1].toInt();
    qDebug() << "[FILE_SUMMARY]" << fileCount << "files";

    int index = 2;
    for (int i = 0; i < fileCount && index + 5 <= parts.size(); i++)
    {
        if (parts[index] == "FILE")
        {
            QString filename = parts[index + 1];
            quint64 allocCount = parts[index + 2].toULongLong();
            quint64 totalMemory = parts[index + 3].toULongLong();
            quint64 leakCount = parts[index + 4].toULongLong();
            quint64 leakedMemory = parts[index + 5].toULongLong();

            qDebug() << "[FILE] name:" << filename
                     << "allocs:" << allocCount << "totalMem:" << bytesToMB(totalMemory) << "MB"
                     << "leaks:" << leakCount << "leakedMem:" << bytesToMB(leakedMemory) << "MB";
            index += 6;
        }
    }
}

void ListenLogic::handleLeakReport(const QStringList &parts)
{
    if (parts.size() < 7)
        return;

    quint64 totalLeaks = parts[1].toULongLong();
    quint64 totalLeakedMemory = parts[2].toULongLong();
    quint64 biggestLeakSize = parts[3].toULongLong();
    QString biggestLeakFile = parts[4];
    QString mostFrequentLeakFile = parts[5];
    quint64 mostFrequentLeakCount = parts[6].toULongLong();

    qDebug() << "[LEAK_REPORT] Total leaks:" << totalLeaks
             << "Total leaked:" << bytesToMB(totalLeakedMemory) << "MB"
             << "Biggest leak:" << bytesToMB(biggestLeakSize) << "MB in" << biggestLeakFile
             << "File with most leaks:" << mostFrequentLeakFile << "count:" << mostFrequentLeakCount;

    if (parts.size() > 7 && parts[7] == "LEAKS_START")
    {
        int leakCount = parts[8].toInt();
        qDebug() << "[LEAKS_START] count:" << leakCount;

        int index = 9;
        for (int i = 0; i < leakCount && index + 6 <= parts.size(); i++)
        {
            if (parts[index] == "LEAK")
            {
                quint64 addr = parts[index + 1].toULongLong();
                quint64 size = parts[index + 2].toULongLong();
                QString file = parts[index + 3];
                int line = parts[index + 4].toInt();
                QString type = parts[index + 5];
                quint64 timestamp = parts[index + 6].toULongLong();

                qDebug() << "[LEAK] addr:" << formatAddress(addr)
                         << "size:" << size << "file:" << file << "line:" << line
                         << "type:" << type << "timestamp:" << timestamp;
                index += 7;
            }
        }
    }
}

void ListenLogic::handleTimelinePoint(const QStringList &parts)
{
    if (parts.size() < 4)
        return;

    quint64 timestamp = parts[1].toULongLong();
    quint64 currentMemory = parts[2].toULongLong();
    quint64 activeAllocations = parts[3].toULongLong();

    qDebug() << "[TIMELINE] Time:" << timestamp << "ms"
             << "Memory:" << bytesToMB(currentMemory) << "MB"
             << "Active allocs:" << activeAllocations;
}

QString ListenLogic::bytesToMB(quint64 bytes)
{
    return QString::number(bytes / (1024.0 * 1024.0), 'f', 2);
}

QString ListenLogic::formatAddress(quint64 addr)
{
    return QString("0x%1").arg(addr, 16, 16, QChar('0'));
}
