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
#include <QLineEdit>
#include <QPushButton>
#include "ListenLogic.h"
#include "profiler_structures.h"
#include <QLineSeries>
#include <QValueAxis>




class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool serverStarted;

private slots:
    void updateGeneralMetrics(const GeneralMetrics &metrics);
    void onStartServerClicked();
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();

private:
    void setupConnectionTab();
    void setupOverviewTab();
    void setupMemoryMapTab();
    void setupAllocationByFileTab();
    void setupMemoryLeaksTab();
    void processData(const QByteArray &data);
    void updateTimelineChart(const TimelinePoint &point);

    // Timeline Chart - AGREGAR ESTAS
    QChart *timelineChart;
    QLineSeries *timelineSeries;
    QValueAxis *axisX;
    QValueAxis *axisY;
    QVector<TimelinePoint> timelineData;
    qint64 startTime;
    const int MAX_POINTS = 100;


    // Server and networking
    QTcpServer *tcpServer;
    QList<QTcpSocket *> clients;
    bool hasClientEverConnected;
    ListenLogic *listenLogic;

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
