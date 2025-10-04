#pragma once
#include <QString>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QStringList>

class ListenLogic
{
public:
    ListenLogic() = default;

    void processData(const QString &keyword, const QByteArray &data);

private:
    void handleLiveUpdate(const QStringList &parts);
    void handleGeneralMetrics(const QStringList &parts);
    void handleMemoryMap(const QStringList &parts);
    void handleFileAllocations(const QStringList &parts);
    void handleLeakReport(const QStringList &parts);
    void handleTimelinePoint(const QStringList &parts);

    // Métodos auxiliares para conversión
    QString bytesToMB(quint64 bytes);
    QString formatAddress(quint64 addr);
};