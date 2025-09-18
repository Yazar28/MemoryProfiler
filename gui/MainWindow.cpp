#include "mainwindow.h"
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QTableWidget>
#include <QLabel>
#include <QChartView>
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Configurar ventana principal
    setWindowTitle("Memory Profiler");
    setMinimumSize(1200, 800);

    // Crear widget de pestañas
    tabWidget = new QTabWidget(this);
    setCentralWidget(tabWidget);

    // Crear las pestañas
    setupOverviewTab();
    setupMemoryMapTab();
    setupAllocationByFileTab();
    setupMemoryLeaksTab();

    // Añadir pestañas al widget
    tabWidget->addTab(overviewTab, "Vista General");
    tabWidget->addTab(memoryMapTab, "Mapa de Memoria");
    tabWidget->addTab(allocationByFileTab, "Asignación por Archivo");
    tabWidget->addTab(memoryLeaksTab, "Memory Leaks");
}

MainWindow::~MainWindow() {
    // Los objetos QObject se eliminan automáticamente por el sistema de parentesco de Qt
}

void MainWindow::setupOverviewTab() {
    overviewTab = new QWidget();
    overviewLayout = new QGridLayout(overviewTab);

    // Panel de Métricas Generales
    QGroupBox *metricsGroup = new QGroupBox("Métricas Generales");
    QGridLayout *metricsLayout = new QGridLayout();

    currentMemoryLabel = new QLabel("Uso actual: 0 MB");
    activeAllocationsLabel = new QLabel("Asignaciones activas: 0");
    memoryLeaksLabel = new QLabel("Memory leaks: 0 MB");
    maxMemoryLabel = new QLabel("Uso máximo: 0 MB");
    totalAllocationsLabel = new QLabel("Total asignaciones: 0");

    metricsLayout->addWidget(currentMemoryLabel, 0, 0);
    metricsLayout->addWidget(activeAllocationsLabel, 0, 1);
    metricsLayout->addWidget(memoryLeaksLabel, 1, 0);
    metricsLayout->addWidget(maxMemoryLabel, 1, 1);
    metricsLayout->addWidget(totalAllocationsLabel, 2, 0, 1, 2);

    metricsGroup->setLayout(metricsLayout);

    // Línea de tiempo (área para gráfico)
    QGroupBox *timelineGroup = new QGroupBox("Línea de Tiempo");
    QVBoxLayout *timelineLayout = new QVBoxLayout();
    timelineChartView = new QChartView();
    timelineChartView->setRenderHint(QPainter::Antialiasing);
    timelineLayout->addWidget(timelineChartView);
    timelineGroup->setLayout(timelineLayout);

    // Resumen (top 3 archivos)
    QGroupBox *summaryGroup = new QGroupBox("Resumen - Top 3 Archivos con Mayor Asignación");
    QVBoxLayout *summaryLayout = new QVBoxLayout();

    topFilesTable = new QTableWidget();
    topFilesTable->setColumnCount(3);
    topFilesTable->setHorizontalHeaderLabels({"Archivo", "Asignaciones", "Memoria (MB)"});
    topFilesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    topFilesTable->setRowCount(3);

    // Llenar con datos de ejemplo
    for (int i = 0; i < 3; ++i) {
        topFilesTable->setItem(i, 0, new QTableWidgetItem("archivo" + QString::number(i+1) + ".cpp"));
        topFilesTable->setItem(i, 1, new QTableWidgetItem("0"));
        topFilesTable->setItem(i, 2, new QTableWidgetItem("0.0"));
    }

    summaryLayout->addWidget(topFilesTable);
    summaryGroup->setLayout(summaryLayout);

    // Organizar en el layout principal
    overviewLayout->addWidget(metricsGroup, 0, 0);
    overviewLayout->addWidget(timelineGroup, 1, 0);
    overviewLayout->addWidget(summaryGroup, 2, 0);

    // Configurar proporciones
    overviewLayout->setRowStretch(1, 3); // La gráfica ocupa más espacio
}

void MainWindow::setupMemoryMapTab() {
    memoryMapTab = new QWidget();
    memoryMapLayout = new QGridLayout(memoryMapTab);

    QGroupBox *memoryMapGroup = new QGroupBox("Mapa de Memoria");
    QVBoxLayout *groupLayout = new QVBoxLayout();

    memoryMapTable = new QTableWidget();
    memoryMapTable->setColumnCount(5);
    memoryMapTable->setHorizontalHeaderLabels({"Dirección", "Tamaño", "Tipo", "Estado", "Archivo"});
    memoryMapTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Añadir algunas filas de ejemplo
    memoryMapTable->setRowCount(5);
    for (int i = 0; i < 5; ++i) {
        memoryMapTable->setItem(i, 0, new QTableWidgetItem("0x" + QString::number(QRandomGenerator::global()->generate(), 16)));
        memoryMapTable->setItem(i, 1, new QTableWidgetItem(QString::number(QRandomGenerator::global()->generate() % 1024) + " B"));
        memoryMapTable->setItem(i, 2, new QTableWidgetItem("int"));
        memoryMapTable->setItem(i, 3, new QTableWidgetItem("Activo"));
        memoryMapTable->setItem(i, 4, new QTableWidgetItem("main.cpp"));
    }

    groupLayout->addWidget(memoryMapTable);
    memoryMapGroup->setLayout(groupLayout);
    memoryMapLayout->addWidget(memoryMapGroup, 0, 0);
}

void MainWindow::setupAllocationByFileTab() {
    allocationByFileTab = new QWidget();
    allocationByFileLayout = new QGridLayout(allocationByFileTab);

    // Gráfico de asignaciones por archivo
    QGroupBox *chartGroup = new QGroupBox("Asignación de Memoria por Archivo");
    QVBoxLayout *chartLayout = new QVBoxLayout();
    allocationChartView = new QChartView();
    allocationChartView->setRenderHint(QPainter::Antialiasing);
    chartLayout->addWidget(allocationChartView);
    chartGroup->setLayout(chartLayout);

    // Tabla detallada
    QGroupBox *tableGroup = new QGroupBox("Detalles por Archivo");
    QVBoxLayout *tableLayout = new QVBoxLayout();
    allocationTable = new QTableWidget();
    allocationTable->setColumnCount(3);
    allocationTable->setHorizontalHeaderLabels({"Archivo", "Asignaciones", "Memoria (MB)"});
    allocationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Datos de ejemplo
    allocationTable->setRowCount(5);
    for (int i = 0; i < 5; ++i) {
        allocationTable->setItem(i, 0, new QTableWidgetItem("archivo" + QString::number(i+1) + ".cpp"));
        allocationTable->setItem(i, 1, new QTableWidgetItem(QString::number(QRandomGenerator::global()->generate() % 100)));
        allocationTable->setItem(i, 2, new QTableWidgetItem(QString::number((QRandomGenerator::global()->generate() % 1000) / 10.0, 'f', 1)));
    }

    tableLayout->addWidget(allocationTable);
    tableGroup->setLayout(tableLayout);

    // Organizar en splitter para redimensionamiento
    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(chartGroup);
    splitter->addWidget(tableGroup);
    splitter->setSizes({400, 200});

    allocationByFileLayout->addWidget(splitter, 0, 0);
}

void MainWindow::setupMemoryLeaksTab() {
    memoryLeaksTab = new QWidget();
    memoryLeaksLayout = new QGridLayout(memoryLeaksTab);

    // Panel de resumen
    QGroupBox *summaryGroup = new QGroupBox("Resumen de Memory Leaks");
    QGridLayout *summaryLayout = new QGridLayout();

    totalLeakedMemoryLabel = new QLabel("Total memoria fugada: 0 MB");
    biggestLeakLabel = new QLabel("Leak más grande: 0 MB (archivo.cpp:123)");
    mostFrequentLeakFileLabel = new QLabel("Archivo con más leaks: archivo.cpp");
    leakRateLabel = new QLabel("Tasa de leaks: 0%");

    summaryLayout->addWidget(totalLeakedMemoryLabel, 0, 0);
    summaryLayout->addWidget(biggestLeakLabel, 0, 1);
    summaryLayout->addWidget(mostFrequentLeakFileLabel, 1, 0);
    summaryLayout->addWidget(leakRateLabel, 1, 1);

    summaryGroup->setLayout(summaryLayout);

    // Gráficos
    QGroupBox *chartsGroup = new QGroupBox("Gráficos de Memory Leaks");
    QGridLayout *chartsLayout = new QGridLayout();

    leaksByFileChartView = new QChartView();
    leaksByFileChartView->setRenderHint(QPainter::Antialiasing);

    leaksDistributionChartView = new QChartView();
    leaksDistributionChartView->setRenderHint(QPainter::Antialiasing);

    leaksTimelineChartView = new QChartView();
    leaksTimelineChartView->setRenderHint(QPainter::Antialiasing);

    chartsLayout->addWidget(leaksByFileChartView, 0, 0);
    chartsLayout->addWidget(leaksDistributionChartView, 0, 1);
    chartsLayout->addWidget(leaksTimelineChartView, 1, 0, 1, 2);

    chartsGroup->setLayout(chartsLayout);

    // Organizar en el layout principal
    memoryLeaksLayout->addWidget(summaryGroup, 0, 0);
    memoryLeaksLayout->addWidget(chartsGroup, 1, 0);

    // Configurar proporciones
    memoryLeaksLayout->setRowStretch(1, 3); // Los gráficos ocupan más espacio
}
