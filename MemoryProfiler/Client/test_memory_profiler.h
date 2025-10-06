#ifndef TEST_MEMORY_PROFILER_H
#define TEST_MEMORY_PROFILER_H

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QDateTime>
#include <QRandomGenerator>
#include <algorithm>
#include "ServerClient.h"
#include "profiler_structures.h"

class MemoryProfilerTest : public QObject
{
    Q_OBJECT

public:
    explicit MemoryProfilerTest(QObject *parent = nullptr);
    bool startTest();

private slots:
    void sendGeneralMetrics();
    void sendTimelinePoint();
    void sendTopFiles();
    void sendMemoryMap();
    void sendMemoryStats();

private:
    Client *client;
    QTimer *metricsTimer;
    QTimer *timelineTimer;
    QTimer *topFilesTimer;
    QTimer *memoryMapTimer;
    QTimer *memoryStatsTimer;
};

#endif // TEST_MEMORY_PROFILER_H
