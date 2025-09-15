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

//QT_CHARTS_USE_NAMESPACE

    class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupOverviewTab();
    void setupMemoryMapTab();
    void setupAllocationByFileTab();
    void setupMemoryLeaksTab();

    // Overview Tab
    QTabWidget *tabWidget;
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
