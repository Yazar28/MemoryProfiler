#pragma once
#include <QObject>
#include <QTimer>
#include <QList>
#include <QString>
#include "./Client/ServerClient.h"
#include "./Client/profiler_structures.h"

class MpLibSim : public QObject
{
    Q_OBJECT
public:
    explicit MpLibSim(QObject* parent = nullptr);

    void setClient(Client* c) { client_ = c; }

    void start();
    void stop();

private slots:
    void tick();

private:
    void sendGeneral();
    void sendTimeline(qint64 nowMs);
    void sendMemoryMap();
    void sendFileSummary();
    void sendLeakStuff();

private:
    Client* client_ = nullptr;

    QTimer  timer_;
    qint64  t0EpochMs_ = 0;
    qint64  lastMs_    = -1;

    // Estado simulado
    int     counter_        = 0;
    double  currentMB_      = 50.0;
    double  targetMB_       = 50.0;
    double  maxMB_          = 50.0;
    int     totalAllocs_    = 0;
    int     activeAllocs_   = 0;

    // Parámetros de la señal
    const int    tickMs_     = 250;
    const double maxStepMB_  = 0.60;
    double       drift_      = 0.02;
    double       noiseAmp_   = 0.35;
    double       waveAmp_    = 1.6;
    double       wavePhase_  = 0.0;
};
