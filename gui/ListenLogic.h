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
    ListenLogic(QObject *parent = nullptr) : QObject(parent) {}

    void processData(const QString &keyword, const QByteArray &data);
    void handleLiveUpdate(const QStringList &parts);
    void handleGeneralMetrics(const QStringList &parts);
    void handleGeneralMetrics(const QByteArray &data);
    void handleMemoryMap(const QStringList &parts);
    void handleFileAllocations(const QStringList &parts);
    void handleLeakReport(const QStringList &parts);
    void handleTimelinePoint(const QStringList &parts);

    // Métodos auxiliares para conversión
    QString bytesToMB(quint64 bytes);
    QString formatAddress(quint64 addr);

signals:
    void generalMetricsUpdated(const GeneralMetrics &metrics);
};
