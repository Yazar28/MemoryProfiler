#include "ListenLogic.h"
#include <QChart>
#include <QBarSeries>
#include <QBarSet>
#include <QLineSeries>
#include <QPieSeries>
#include "profiler_structures.h"

// procesa los datos recibidos y llama a los manejadores adecuados
void ListenLogic::processData(const QString &keyword, const QByteArray &data)
{
    qDebug() << "=== PROCESANDO DATOS ===";
    qDebug() << "Keyword:" << keyword;
    qDebug() << "Tamaño datos:" << data.size() << "bytes";
    // Procesar datos binarios (estructuras)
    if (keyword == "GENERAL_METRICS") // LLamam al metodo que deserializa y emite la señal para actualizar la UI en la parte de Metricas Generales
    {
        handleGeneralMetrics(data);
        return;
    }
    else if (keyword == "TIMELINE_POINT") // llamam al metodo que deserializa y emite la señal para actualizar la UI en la parte de Timeline
    {
        handleTimelinePoint(data);
        return;
    }
    else if (keyword == "TOP_FILES")
    {
        handleTopFile(data);
        return;
    }
    else
    {
        qDebug() << "✗ Keyword desconocido para datos binarios:" << keyword;
        return;
    }

    // Para otros keywords, procesar como string con separadores '|'
    QString dataStr = QString::fromUtf8(data);
    QStringList parts = dataStr.split('|');

    if (keyword == "LIVE_UPDATE") // llamam al metodo que deserializa y emite la señal para actualizar la UI en la parte de Live Updates
    {
        handleLiveUpdate(parts);
    }
    if (keyword == "MEMORY_MAP") // llamam al metodo que deserializa y emite la señal para actualizar la UI en la parte de Memory Map
    {
        handleMemoryMap(parts);
    }
    if (keyword == "FILE_ALLOCATIONS") // llamam al metodo que deserializa y emite la señal para actualizar la UI en la parte de File Allocations
    {
        handleFileAllocations(parts);
    }
    if (keyword == "LEAK_REPORT") // llamam al metodo que deserializa y emite la señal para actualizar la UI en la parte de Leak Report
    {
        handleLeakReport(parts);
    }
    else
    {
        qDebug() << "✗ Keyword desconocido para datos string:" << keyword;
    }
}
// Maneja actualizaciones en vivo (formato Qstring)
void ListenLogic::handleLiveUpdate(const QStringList &parts)
{
    if (parts.size() < 2) // mínimo 2 partes necesarias
        return;

    if (parts[0] == "ALLOC" && parts.size() >= 6) // esto hace q se vean las asignaciones en vivo
    {
        quint64 addr = parts[1].toULongLong();
        quint64 size = parts[2].toULongLong();
        QString file = parts[3];
        int line = parts[4].toInt();
        QString type = parts[5];

        qDebug() << "[LIVE] ALLOC addr:" << formatAddress(addr)
                 << "size:" << size << "file:" << file << "line:" << line << "type:" << type;
    }
    if (parts[0] == "FREE" && parts.size() >= 2) // esto hace q se vean las liberaciones en vivo
    {
        quint64 addr = parts[1].toULongLong();
        qDebug() << "[LIVE] FREE addr:" << formatAddress(addr);
    }
}
// Sobrecarga del operador >> para deserializar GeneralMetrics
QDataStream &operator>>(QDataStream &stream, GeneralMetrics &metrics)
{
    stream >> metrics.currentUsageMB >> metrics.activeAllocations >> metrics.memoryLeaksMB >> metrics.maxMemoryMB >> metrics.totalAllocations;
    return stream;
}
// Sobrecarga del operador >> para deserializar TimelinePoint
void ListenLogic::handleGeneralMetrics(const QByteArray &data)
{
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian); // Asegura el orden de bytes correcto

    GeneralMetrics metrics; // Estructura para almacenar las métricas
    stream >> metrics;

    if (stream.status() != QDataStream::Ok) // Verifica si la deserialización fue exitosa
    {
        // Muestra un error en la consola si falla
        qDebug() << "✗ Error: No se pudo deserializar GeneralMetrics";
        qDebug() << "Bytes recibidos:" << data.size();
        qDebug() << "Stream status:" << stream.status();
        return;
    }
    // Muestra las métricas en la consola para depuración
    qDebug() << "✓ Métricas deserializadas CORRECTAMENTE";
    qDebug() << "  Current:" << metrics.currentUsageMB << "MB";
    qDebug() << "  Active:" << metrics.activeAllocations;
    qDebug() << "  Leaks:" << metrics.memoryLeaksMB << "MB";
    qDebug() << "  Max:" << metrics.maxMemoryMB << "MB";
    qDebug() << "  Total:" << metrics.totalAllocations;

    emit generalMetricsUpdated(metrics); // Emite la señal para actualizar la UI
}
// Sobrecarga del operador >> para deserializar TopFile
void ListenLogic::handleTopFile(const QByteArray &data)
{
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);

    QVector<TopFile> topFiles;
    quint32 size;
    stream >> size;

    for (quint32 i = 0; i < size; ++i)
    {
        TopFile file;
        stream >> file.filename >> file.allocations >> file.memoryMB;
        topFiles.append(file);
    }

    if (stream.status() == QDataStream::Ok)
    {
        qDebug() << "✓ TopFiles recibido - Archivos:" << topFiles.size();
        emit topFilesUpdated(topFiles);
    }
    else
    {
        qDebug() << "✗ Error deserializando TopFiles";
    }
}
// Maneja el mapa de memoria (formato QString)
void ListenLogic::handleMemoryMap(const QStringList &parts) // Maneja el mapa de memoria (formato QString)
{
    if (parts.size() < 2 || parts[0] != "MEMORY_MAP_START") // mínimo 2 partes necesarias
        return;

    int blockCount = parts[1].toInt();                               // número de bloques
    qDebug() << "[MEM_MAP] Starting with" << blockCount << "blocks"; // Log del inicio del mapa de memoria en la consola

    int index = 2;
    for (int i = 0; i < blockCount && index + 5 <= parts.size(); i++)
    {
        if (parts[index] == "BLOCK") // cada bloque empieza con la palabra "BLOCK"
        {
            // Extrae los detalles del bloque
            quint64 addr = parts[index + 1].toULongLong();
            quint64 size = parts[index + 2].toULongLong();
            QString type = parts[index + 3];
            QString file = parts[index + 4];
            int line = parts[index + 5].toInt();

            qDebug() << "[BLOCK] addr:" << formatAddress(addr) // Formatea la dirección en hexadecimal
                     << "size:" << size << "type:" << type << "file:" << file << "line:" << line;
            index += 6; // Mueve el índice al siguiente bloque
        }
    }
}
// Maneja las asignaciones por archivo (formato QString)
void ListenLogic::handleFileAllocations(const QStringList &parts)
{
    // Maneja las asignaciones por archivo (formato QString)
    if (parts.size() < 2 || parts[0] != "FILE_SUMMARY_START")
        return;

    int fileCount = parts[1].toInt();
    qDebug() << "[FILE_SUMMARY]" << fileCount << "files";

    int index = 2;
    for (int i = 0; i < fileCount && index + 5 <= parts.size(); i++)
    {
        if (parts[index] == "FILE")
        {
            // Extrae los detalles del archivo
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
// Maneja el informe de fugas (formato QString)
void ListenLogic::handleLeakReport(const QStringList &parts)
{
    if (parts.size() < 7) // mínimo 7 partes necesarias
        return;
    // Extrae los detalles del informe de fugas
    quint64 totalLeaks = parts[1].toULongLong();
    quint64 totalLeakedMemory = parts[2].toULongLong();
    quint64 biggestLeakSize = parts[3].toULongLong();
    QString biggestLeakFile = parts[4];
    QString mostFrequentLeakFile = parts[5];
    quint64 mostFrequentLeakCount = parts[6].toULongLong();
    // Muestra el resumen del informe de fugas en la consola
    qDebug() << "[LEAK_REPORT] Total leaks:" << totalLeaks
             << "Total leaked:" << bytesToMB(totalLeakedMemory) << "MB"
             << "Biggest leak:" << bytesToMB(biggestLeakSize) << "MB in" << biggestLeakFile
             << "File with most leaks:" << mostFrequentLeakFile << "count:" << mostFrequentLeakCount;

    if (parts.size() > 7 && parts[7] == "LEAKS_START") // Detalles de fugas individuales
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
// Maneja un punto de la línea de tiempo (estructura binaria)
void ListenLogic::handleTimelinePoint(const QByteArray &data)
{
    QDataStream stream(data); // Crea un QDataStream para leer los datos binarios
    stream.setByteOrder(QDataStream::BigEndian);

    TimelinePoint point; // Estructura para almacenar el punto de la línea de tiempo
    stream >> point.timestamp >> point.memoryMB;
    // Deserializa los campos
    if (stream.status() == QDataStream::Ok)
    {
        qDebug() << "✓ TimelinePoint recibido - Time:" << point.timestamp << "Memory:" << point.memoryMB << "MB";
        emit timelinePointUpdated(point); // <- AGREGAR ESTA LÍNEA
    }
    else
    {
        qDebug() << "✗ Error deserializando TimelinePoint";
    }
}
// Convierte bytes a MB con dos decimales
QString ListenLogic::bytesToMB(quint64 bytes)
{
    return QString::number(bytes / (1024.0 * 1024.0), 'f', 2);
}
// Formatea una dirección como hexadecimal
QString ListenLogic::formatAddress(quint64 addr)
{
    return QString("0x%1").arg(addr, 16, 16, QChar('0'));
}
