#include "ListenLogic.h"
#include <QChart>
#include <QBarSeries>
#include <QBarSet>
#include <QLineSeries>
#include <QPieSeries>

void ListenLogic::processData(const QString &keyword, const QByteArray &data)
{
    QString dataStr = QString::fromUtf8(data);
    QStringList parts = dataStr.split('|');

    qDebug() << "Received keyword:" << keyword << "Data:" << dataStr;

    // LIVE UPDATES - Alloc/Free en tiempo real
    if (keyword == "LIVE_UPDATE")
    {
        handleLiveUpdate(parts);
    }

    // MÉTRICAS GENERALES - Para la pestaña de vista general
    if (keyword == "GENERAL_METRICS")
    {
        handleGeneralMetrics(parts);
    }

    // MAPA DE MEMORIA - Para la pestaña de mapa de memoria
    if (keyword == "MEMORY_MAP")
    {
        handleMemoryMap(parts);
    }

    // ASIGNACIONES POR ARCHIVO - Para la pestaña de archivos fuente
    if (keyword == "FILE_ALLOCATIONS")
    {
        handleFileAllocations(parts);
    }

    // REPORTE DE LEAKS - Para la pestaña de memory leaks
    if (keyword == "LEAK_REPORT")
    {
        handleLeakReport(parts);
    }

    // LÍNEA DE TIEMPO - Para el gráfico temporal
    if (keyword == "TIMELINE_POINT")
    {
        handleTimelinePoint(parts);
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

        // Aquí emitir señal para actualizar la interfaz
        // emit allocReceived(addr, size, file, line, type);
    }
    if (parts[0] == "FREE" && parts.size() >= 2)
    {
        quint64 addr = parts[1].toULongLong();
        qDebug() << "[LIVE] FREE addr:" << formatAddress(addr);

        // Aquí emitir señal para actualizar la interfaz
        // emit freeReceived(addr);
    }
}

void ListenLogic::handleGeneralMetrics(const QStringList &parts)
{
    if (parts.size() < 5)
        return;

    quint64 totalAllocs = parts[0].toULongLong();
    quint64 activeAllocs = parts[1].toULongLong();
    quint64 currentMem = parts[2].toULongLong();
    quint64 peakMem = parts[3].toULongLong();
    quint64 leakedMem = parts[4].toULongLong();

    qDebug() << "[METRICS] TotalAllocs:" << totalAllocs
             << "ActiveAllocs:" << activeAllocs
             << "CurrentMem:" << bytesToMB(currentMem) << "MB"
             << "PeakMem:" << bytesToMB(peakMem) << "MB"
             << "LeakedMem:" << bytesToMB(leakedMem) << "MB";

    // Aquí emitir señal para actualizar la pestaña de vista general
    // emit generalMetricsUpdated(totalAllocs, activeAllocs, currentMem, peakMem, leakedMem);
}

void ListenLogic::handleMemoryMap(const QStringList &parts)
{
    if (parts.size() < 2 || parts[0] != "MEMORY_MAP_START")
        return;

    int blockCount = parts[1].toInt();
    qDebug() << "[MEM_MAP] Starting with" << blockCount << "blocks";

    // Aquí procesaríamos cada bloque y actualizaríamos la tabla del mapa de memoria
    // emit memoryMapCleared();

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

            // emit memoryBlockAdded(addr, size, type, file, line);
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

    // Aquí procesaríamos los archivos para la pestaña de asignación por archivo
    // emit fileSummaryCleared();

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

            // emit fileSummaryAdded(filename, allocCount, totalMemory, leakCount, leakedMemory);
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

    // Aquí procesaríamos los leaks detallados para la pestaña de memory leaks
    // emit leakReportUpdated(totalLeaks, totalLeakedMemory, biggestLeakSize, biggestLeakFile,
    //                       mostFrequentLeakFile, mostFrequentLeakCount);

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

                // emit leakDetailAdded(addr, size, file, line, type, timestamp);
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

    // Aquí emitir señal para actualizar la línea de tiempo
    // emit timelinePointAdded(timestamp, currentMemory, activeAllocations);
}

QString ListenLogic::bytesToMB(quint64 bytes)
{
    return QString::number(bytes / (1024.0 * 1024.0), 'f', 2);
}

QString ListenLogic::formatAddress(quint64 addr)
{
    return QString("0x%1").arg(addr, 16, 16, QChar('0'));
}