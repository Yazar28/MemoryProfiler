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
#include <QtNetwork/QTcpServer>  // Cambiado de QTcpSocket a QTcpServer
#include <QtNetwork/QTcpSocket>
#include <QHostAddress>
#include <QMessageBox>
#include <QString>
#include <QByteArray>
#include <QStackedWidget>
#include <QList>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartServerClicked();  // Cambiado de onConnectButtonClicked
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();
private:
    void setupConnectionTab();
    void setupOverviewTab();          // Asegúrate de que esté declarada
    void setupMemoryMapTab();         // Asegúrate de que esté declarada
    void setupAllocationByFileTab();  // Asegúrate de que esté declarada
    void setupMemoryLeaksTab();       // Asegúrate de que esté declarada
    void processData(const QByteArray &data);
    QTcpServer *tcpServer;        // Servidor TCP
    QList<QTcpSocket*> clients;   // Lista de clientes conectados

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
