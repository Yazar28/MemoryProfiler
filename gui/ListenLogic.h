#pragma once
#include <QString>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QStringList>
#include "profiler_structures.h"
#include <QObject>

class ListenLogic : public QObject
{
    Q_OBJECT

public:
    // Constructor
    ListenLogic(QObject *parent = nullptr) : QObject(parent) {}
    void processData(const QString &keyword, const QByteArray &data); // procesa los datos recibidos y llama a los manejadores adecuados

    // Métodos auxiliares para conversión
    QString bytesToMB(quint64 bytes);    // Convierte bytes a megabytes en QString
    QString formatAddress(quint64 addr); // Formatea una dirección de memoria como cadena hexadecimal

signals:                                                                                 // las signals son funciones que emiten señales a otras partes del programa
    void generalMetricsUpdated(const GeneralMetrics &metrics);                           // Señal para actualizar las métricas generales
    void timelinePointUpdated(const TimelinePoint &point);                               // señal para actualizar el timeline
    void topFilesUpdated(const QVector<TopFile> &topFiles);                              // señal para actualizar los archivos principales
    void basicMemoryMapUpdated(const QVector<MemoryMapTypes::BasicMemoryBlock> &blocks); // señal para actualizar el mapa de memoria básico
    void memoryStatsUpdated(const MemoryMapTypes::MemoryStats &stats);                   //
    void memoryEventReceived(const MemoryEvent &event);                                  // Nueva señal para eventos individua
    void fileAllocationSummaryUpdated(const QVector<FileAllocationSummary> &fileAllocs); //
    void leakSummaryUpdated(const LeakSummary &summary);
    void leaksByFileUpdated(const QVector<LeakByFile> &leaksByFile);
    void leakTimelineUpdated(const QVector<LeakTimelinePoint> &leakTimeline);

private:
    // Manejadores para diferentes tipos de datos
    void handleLiveUpdate(const QStringList &parts);          // Maneja actualizaciones en vivo (formato Qstring)
    void handleGeneralMetrics(const QByteArray &data);        // Sobrecarga del operador >> para deserializar GeneralMetrics
    void handleMemoryMap(const QStringList &parts);           // Maneja el mapa de memoria (formato QString)
    void handleFileAllocations(const QStringList &parts);     // Maneja las asignaciones por archivo (formato QString)
    void handleLeakReport(const QStringList &parts);          // Maneja el informe de fugas (formato QString)
    void handleTimelinePoint(const QByteArray &data);         // Maneja un punto de la línea de tiempo (estructura binaria)
    void handleTopFile(const QByteArray &data);               // Sobrecarga del operador >> para deserializar TopFile
    void handleBasicMemoryMap(const QByteArray &data);        // Maneja el mapa de memoria básico (estructura binaria)
    void handleMemoryStats(const QByteArray &data);           // Maneja las estadísticas de memoria (estructura binaria)
    void handleMemoryEvent(const QByteArray &data);           //
    void handleFileAllocationSummary(const QByteArray &data); //
    void handleLeakSummary(const QByteArray &data);
    void handleLeaksByFile(const QByteArray &data);
    void handleLeakTimeline(const QByteArray &data);
};
