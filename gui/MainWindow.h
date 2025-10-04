#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTableWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QHostAddress>
#include <QMessageBox>
#include <QString>
#include <QByteArray>
#include <QStackedWidget>
#include <QList>
#include "ListenLogic.h" // Incluir el nuevo header

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartServerClicked();
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();

private:
    // ... otras variables existentes ...
    bool hasClientEverConnected; // Nueva variable
    void setupConnectionTab();
    void setupOverviewTab();
    void setupMemoryMapTab();
    void setupAllocationByFileTab();
    void setupMemoryLeaksTab();
    void processData(const QByteArray &data);
    // Slots para las señales de ListenLogic (los implementaremos después)
    void onGeneralMetricsUpdated(quint64 totalAllocs, quint64 activeAllocs,
                                 quint64 currentMem, quint64 peakMem, quint64 leakedMem);
    void onTimelinePointAdded(quint64 timestamp, quint64 currentMemory, quint64 activeAllocations);

    QTcpServer *tcpServer;
    QList<QTcpSocket *> clients;
    ListenLogic *listenLogic; // Nueva instancia de ListenLogic

    // Connection Tab
    QWidget *connectionTab;
    QLineEdit *portInput;
    QPushButton *startServerButton;
    QLabel *serverStatusLabel;
    QLabel *clientsConnectedLabel;

    // Main tabs container
    QStackedWidget *mainContainer;
    QTabWidget *tabWidget;

    // Overview Tab
    QWidget *overviewTab;
    QGridLayout *overviewLayout;
    QLabel *currentMemoryLabel;
    QLabel *activeAllocationsLabel;
    QLabel *memoryLeaksLabel;
    QLabel *maxMemoryLabel;
    QLabel *totalAllocationsLabel;
    QChartView *timelineChartView;
    QTableWidget *topFilesTable;

    // Memory Map Tab
    QWidget *memoryMapTab;
    QGridLayout *memoryMapLayout;
    QTableWidget *memoryMapTable;

    // Allocation by File Tab
    QWidget *allocationByFileTab;
    QGridLayout *allocationByFileLayout;
    QChartView *allocationChartView;
    QTableWidget *allocationTable;

    // Memory Leaks Tab
    QWidget *memoryLeaksTab;
    QGridLayout *memoryLeaksLayout;
    QLabel *totalLeakedMemoryLabel;
    QLabel *biggestLeakLabel;
    QLabel *mostFrequentLeakFileLabel;
    QLabel *leakRateLabel;
    QChartView *leaksByFileChartView;
    QChartView *leaksDistributionChartView;
    QChartView *leaksTimelineChartView;
};

#endif // MAINWINDOW_H
