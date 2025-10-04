#include "mainwindow.h"
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QTableWidget>
#include <QLabel>
#include <QChartView>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

    // Inicializar ListenLogic
    //listenLogic = new ListenLogic(this);
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);
    hasClientEverConnected = false; // Inicializar en falso
    // Configurar ventana principal
    setWindowTitle("Memory Profiler Server");
    setMinimumSize(1200, 800);

    // Crear contenedor principal con pila de widgets
    mainContainer = new QStackedWidget(this);
    setCentralWidget(mainContainer);

    // Crear la pestaña de conexión
    setupConnectionTab();

    // Crear las pestañas principales (quitamos el "void" que estaba antes de setupOverviewTab)
    setupOverviewTab();
    setupMemoryMapTab();
    setupAllocationByFileTab();
    setupMemoryLeaksTab();

    // Crear el widget de pestañas principales
    tabWidget = new QTabWidget();
    tabWidget->addTab(overviewTab, "Vista General");
    tabWidget->addTab(memoryMapTab, "Mapa de Memoria");
    tabWidget->addTab(allocationByFileTab, "Asignación por Archivo");
    tabWidget->addTab(memoryLeaksTab, "Memory Leaks");

    // Añadir ambos a la pila
    mainContainer->addWidget(connectionTab); // Índice 0
    mainContainer->addWidget(tabWidget);     // Índice 1

    // Mostrar solo la pestaña de conexión al inicio
    mainContainer->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    if (tcpServer)
    {
        tcpServer->close();
        delete tcpServer;
    }

    // Cerrar todas las conexiones de clientes
    for (QTcpSocket *client : clients)
    {
        client->close();
        client->deleteLater();
    }
}

void MainWindow::setupConnectionTab()
{
    connectionTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(connectionTab);

    QGroupBox *connectionGroup = new QGroupBox("Configuración del Servidor");
    QVBoxLayout *groupLayout = new QVBoxLayout();

    // Campo de entrada para el puerto
    QLabel *portLabel = new QLabel("Puerto del servidor:");
    portInput = new QLineEdit();
    portInput->setPlaceholderText("Ej: 8080");
    portInput->setText("8080");

    // Botón para iniciar servidor
    startServerButton = new QPushButton("Iniciar Servidor");
    connect(startServerButton, &QPushButton::clicked, this, &MainWindow::onStartServerClicked);

    // Etiquetas de estado
    serverStatusLabel = new QLabel("Servidor detenido");
    serverStatusLabel->setAlignment(Qt::AlignCenter);

    clientsConnectedLabel = new QLabel("Clientes conectados: 0");
    clientsConnectedLabel->setAlignment(Qt::AlignCenter);

    groupLayout->addWidget(portLabel);
    groupLayout->addWidget(portInput);
    groupLayout->addWidget(startServerButton);
    groupLayout->addWidget(serverStatusLabel);
    groupLayout->addWidget(clientsConnectedLabel);

    connectionGroup->setLayout(groupLayout);
    mainLayout->addWidget(connectionGroup);
    mainLayout->addStretch();
}

void MainWindow::onStartServerClicked()
{
    if (tcpServer->isListening())
    {
        // Detener el servidor
        tcpServer->close();
        startServerButton->setText("Iniciar Servidor");
        serverStatusLabel->setText("Servidor detenido");
    }
    else
    {
        // Iniciar el servidor
        bool ok;
        int port = portInput->text().toInt(&ok);

        if (!ok || port <= 0 || port > 65535)
        {
            QMessageBox::warning(this, "Error", "Puerto no válido");
            return;
        }

        if (tcpServer->listen(QHostAddress::Any, port))
        {
            startServerButton->setText("Detener Servidor");
            serverStatusLabel->setText("Servidor iniciado en puerto " + QString::number(port));
        }
        else
        {
            QMessageBox::warning(this, "Error", "No se pudo iniciar el servidor: " + tcpServer->errorString());
        }
    }
}

void MainWindow::onNewConnection()
{
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &MainWindow::onClientDisconnected);

    clients.append(clientSocket);
    clientsConnectedLabel->setText("Clientes conectados: " + QString::number(clients.size()));

    // Mostrar las pestañas principales cuando se conecta el primer cliente
    if (clients.size() == 1)
    {
        hasClientEverConnected = true; // Marcar que ha habido al menos una conexión
        mainContainer->setCurrentIndex(1);
    }
}

void MainWindow::onClientDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (clientSocket)
    {
        clients.removeOne(clientSocket);
        clientSocket->deleteLater();
        clientsConnectedLabel->setText("Clientes conectados: " + QString::number(clients.size()));

        // Solo volver a conexión si nunca hubo un cliente conectado
        if (clients.isEmpty() && !hasClientEverConnected)
        {
            mainContainer->setCurrentIndex(0);
        }
        // Si hasClientEverConnected es true, mantener en las pestañas principales
    }
}

void MainWindow::onReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (!clientSocket)
        return;

    QByteArray data = clientSocket->readAll();
    processData(data);
}

void MainWindow::processData(const QByteArray &data)
{
    qDebug() << "=== DATOS RECIBIDOS DEL CLIENTE ===";
    qDebug() << "Tamaño total:" << data.size() << "bytes";
    qDebug() << "Datos en crudo:" << data.toHex();

    // Intentar parsear el formato del cliente: [keyword_len][data_len][keyword][data]
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);

    quint16 keywordLen;
    quint32 dataLen;

    // Leer las longitudes
    if (stream.readRawData(reinterpret_cast<char*>(&keywordLen), sizeof(keywordLen)) != sizeof(keywordLen)) {
        qDebug() << "✗ Error: No se pudo leer la longitud del keyword";
        return;
    }

    if (stream.readRawData(reinterpret_cast<char*>(&dataLen), sizeof(dataLen)) != sizeof(dataLen)) {
        qDebug() << "✗ Error: No se pudo leer la longitud de los datos";
        return;
    }

    qDebug() << "Longitud del keyword:" << keywordLen << "bytes";
    qDebug() << "Longitud de los datos:" << dataLen << "bytes";

    // Leer el keyword
    QByteArray keywordBytes(keywordLen, 0);
    if (stream.readRawData(keywordBytes.data(), keywordLen) != keywordLen) {
        qDebug() << "✗ Error: No se pudo leer el keyword";
        return;
    }

    QString keyword = QString::fromUtf8(keywordBytes);
    qDebug() << "Keyword recibido:" << keyword;

    // Leer los datos
    QByteArray receivedData(dataLen, 0);
    if (stream.readRawData(receivedData.data(), dataLen) != dataLen) {
        qDebug() << "✗ Error: No se pudo leer los datos completos";
        return;
    }

    qDebug() << "Datos recibidos:" << receivedData;
    qDebug() << "Datos en hexadecimal:" << receivedData.toHex();

    // Procesar según el keyword usando ListenLogic
    if (listenLogic) {
        listenLogic->processData(keyword, receivedData);
    } else {
        qDebug() << "✗ Error: ListenLogic no está inicializado";
    }

    qDebug() << "=== FIN DE DATOS RECIBIDOS ===";

    // También mostrar en la barra de estado
    statusBar()->showMessage("Datos recibidos: " + keyword + " (" + QString::number(data.size()) + " bytes)");
}

void MainWindow::setupOverviewTab()
{
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

    // Dejar la tabla vacía (sin datos aleatorios)
    summaryLayout->addWidget(topFilesTable);
    summaryGroup->setLayout(summaryLayout);

    // Organizar en el layout principal
    overviewLayout->addWidget(metricsGroup, 0, 0);
    overviewLayout->addWidget(timelineGroup, 1, 0);
    overviewLayout->addWidget(summaryGroup, 2, 0);

    // Configurar proporciones
    overviewLayout->setRowStretch(1, 3); // La gráfica ocupa más espacio
}

void MainWindow::setupMemoryMapTab()
{
    memoryMapTab = new QWidget();
    memoryMapLayout = new QGridLayout(memoryMapTab);

    QGroupBox *memoryMapGroup = new QGroupBox("Mapa de Memoria");
    QVBoxLayout *groupLayout = new QVBoxLayout();

    memoryMapTable = new QTableWidget();
    memoryMapTable->setColumnCount(5);
    memoryMapTable->setHorizontalHeaderLabels({"Dirección", "Tamaño", "Tipo", "Estado", "Archivo"});
    memoryMapTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Dejar la tabla vacía (sin datos aleatorios)
    memoryMapTable->setRowCount(5);

    groupLayout->addWidget(memoryMapTable);
    memoryMapGroup->setLayout(groupLayout);
    memoryMapLayout->addWidget(memoryMapGroup, 0, 0);
}

void MainWindow::setupAllocationByFileTab()
{
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

    // Dejar la tabla vacía (sin datos aleatorios)
    allocationTable->setRowCount(5);

    tableLayout->addWidget(allocationTable);
    tableGroup->setLayout(tableLayout);

    // Organizar en splitter para redimensionamiento
    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(chartGroup);
    splitter->addWidget(tableGroup);
    splitter->setSizes({400, 200});

    allocationByFileLayout->addWidget(splitter, 0, 0);
}

void MainWindow::setupMemoryLeaksTab()
{
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
