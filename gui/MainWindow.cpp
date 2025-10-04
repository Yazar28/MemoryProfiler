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
#include <QMessageBox>
#include <QDataStream>
#include "ListenLogic.h"
#include "profiler_structures.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Inicializar ListenLogic
    listenLogic = new ListenLogic(this);
    connect(listenLogic, &ListenLogic::generalMetricsUpdated,
            this, &MainWindow::updateGeneralMetrics);

    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);
    serverStarted = false;  // Inicializar como false


    // Configurar ventana principal
    setWindowTitle("Memory Profiler Server");
    setMinimumSize(1200, 800);

    // Crear contenedor principal con pila de widgets
    mainContainer = new QStackedWidget(this);
    setCentralWidget(mainContainer);

    // Crear la pestaña de conexión
    setupConnectionTab();

    // Crear las pestañas principales
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
    serverStatusLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    clientsConnectedLabel = new QLabel("Clientes conectados: 0");
    clientsConnectedLabel->setAlignment(Qt::AlignCenter);
    clientsConnectedLabel->setTextInteractionFlags(Qt::NoTextInteraction);

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
        // Detener el servidor y limpiar clientes
        tcpServer->close();
        for (QTcpSocket *client : clients) {
            client->close();
            client->deleteLater();
        }
        clients.clear();

        startServerButton->setText("Iniciar Servidor");
        serverStatusLabel->setText("Servidor detenido");
        clientsConnectedLabel->setText("Clientes conectados: 0");
        portInput->setEnabled(true);

        serverStarted = false;

        // Volver a la pestaña de conexión
        mainContainer->setCurrentIndex(0);
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
            clientsConnectedLabel->setText("Clientes conectados: 0");
            portInput->setEnabled(false);

            serverStarted = true;

            // Ir directamente a las pestañas principales
            mainContainer->setCurrentIndex(1);

            qDebug() << "✓ Servidor iniciado en puerto" << port << "- Esperando conexiones...";
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

    qDebug() << "✓ Nuevo cliente conectado. Total:" << clients.size();
    qDebug() << "  Dirección:" << clientSocket->peerAddress().toString();
    qDebug() << "  Puerto:" << clientSocket->peerPort();

    // Mostrar las pestañas principales si no estaban visibles
    if (serverStarted && mainContainer->currentIndex() == 0) {
        mainContainer->setCurrentIndex(1);
    }

    statusBar()->showMessage("Nuevo cliente conectado - Total: " + QString::number(clients.size()));
}

void MainWindow::onClientDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (clientSocket)
    {
        clients.removeOne(clientSocket);
        clientSocket->deleteLater();
        clientsConnectedLabel->setText("Clientes conectados: " + QString::number(clients.size()));

        qDebug() << "✗ Cliente desconectado. Clientes restantes:" << clients.size();

        // Mostrar en status bar
        if (clients.isEmpty()) {
            statusBar()->showMessage("Todos los clientes desconectados - Servidor sigue escuchando");
        } else {
            statusBar()->showMessage("Cliente desconectado - Clientes activos: " + QString::number(clients.size()));
        }

        // IMPORTANTE: No volver a la pestaña de conexión
        // El servidor sigue activo y escuchando nuevas conexiones
    }
}
void MainWindow::onReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (!clientSocket)
        return;

    QByteArray data = clientSocket->readAll();

    // Opcional: Identificar de qué cliente vienen los datos
    QString clientInfo = clientSocket->peerAddress().toString() + ":" +
                         QString::number(clientSocket->peerPort());

    qDebug() << "📨 Datos recibidos de cliente:" << clientInfo;
    qDebug() << "Tamaño:" << data.size() << "bytes";

    processData(data);
}
void MainWindow::processData(const QByteArray &data)
{
    static QByteArray buffer;
    buffer.append(data);

    QDataStream stream(&buffer, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::BigEndian); // Debe coincidir con el cliente

    while (buffer.size() >= sizeof(quint16) + sizeof(quint32)) {
        // Guardar la posición actual
        qint64 startPos = stream.device()->pos();

        quint16 keywordLen;
        quint32 dataLen;

        // Leer usando operadores >> en lugar de readRawData
        stream >> keywordLen >> dataLen;

        if (stream.status() != QDataStream::Ok) {
            qDebug() << "✗ Error: No se pudieron leer las longitudes";
            buffer.clear();
            return;
        }

        qDebug() << "Longitud del keyword:" << keywordLen << "bytes";
        qDebug() << "Longitud de los datos:" << dataLen << "bytes";

        // Verificar si tenemos todos los datos necesarios
        quint64 totalNeeded = sizeof(keywordLen) + sizeof(dataLen) + keywordLen + dataLen;
        if (buffer.size() - startPos < totalNeeded) {
            // No tenemos todos los datos aún, restaurar posición y esperar más
            stream.device()->seek(startPos);
            buffer = buffer.mid(startPos);
            return;
        }

        // Leer el keyword
        QByteArray keywordBytes(keywordLen, 0);
        if (stream.readRawData(keywordBytes.data(), keywordLen) != keywordLen) {
            qDebug() << "✗ Error: No se pudo leer el keyword";
            buffer.clear();
            return;
        }

        QString keyword = QString::fromUtf8(keywordBytes);
        qDebug() << "Keyword recibido:" << keyword;

        // Leer los datos
        QByteArray receivedData(dataLen, 0);
        if (stream.readRawData(receivedData.data(), dataLen) != dataLen) {
            qDebug() << "✗ Error: No se pudo leer los datos completos";
            buffer.clear();
            return;
        }

        qDebug() << "Datos recibidos:" << receivedData.size() << "bytes";

        // Procesar usando ListenLogic
        if (listenLogic) {
            listenLogic->processData(keyword, receivedData);
        } else {
            qDebug() << "✗ Error: ListenLogic no está inicializado";
        }

        // Eliminar los datos procesados del buffer
        qint64 currentPos = stream.device()->pos();
        buffer = buffer.mid(currentPos);

        // Reiniciar el stream con el buffer actualizado
        stream.device()->seek(0);
    }
    statusBar()->showMessage("Datos procesados - Clientes: " +
                             QString::number(clients.size()) +
                             " - Última actualización: " +
                             QDateTime::currentDateTime().toString("hh:mm:ss"));
}

void MainWindow::setupOverviewTab()
{
    overviewTab = new QWidget();
    overviewLayout = new QGridLayout(overviewTab);

    // Panel de Métricas Generales
    QGroupBox *metricsGroup = new QGroupBox("Métricas Generales");
    QGridLayout *metricsLayout = new QGridLayout();

    currentMemoryLabel = new QLabel("Uso actual: 0 MB");
    currentMemoryLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    activeAllocationsLabel = new QLabel("Asignaciones activas: 0");
    activeAllocationsLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    memoryLeaksLabel = new QLabel("Memory leaks: 0 MB");
    memoryLeaksLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    maxMemoryLabel = new QLabel("Uso máximo: 0 MB");
    maxMemoryLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    totalAllocationsLabel = new QLabel("Total asignaciones: 0");
    totalAllocationsLabel->setTextInteractionFlags(Qt::NoTextInteraction);

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
    topFilesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    summaryLayout->addWidget(topFilesTable);
    summaryGroup->setLayout(summaryLayout);

    // Organizar en el layout principal
    overviewLayout->addWidget(metricsGroup, 0, 0);
    overviewLayout->addWidget(timelineGroup, 1, 0);
    overviewLayout->addWidget(summaryGroup, 2, 0);

    // Configurar proporciones
    overviewLayout->setRowStretch(1, 3);
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
    memoryMapTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memoryMapTable->setRowCount(0);

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
    allocationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    allocationTable->setRowCount(0);

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
    totalLeakedMemoryLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    biggestLeakLabel = new QLabel("Leak más grande: 0 MB (archivo.cpp:123)");
    biggestLeakLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    mostFrequentLeakFileLabel = new QLabel("Archivo con más leaks: archivo.cpp");
    mostFrequentLeakFileLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    leakRateLabel = new QLabel("Tasa de leaks: 0%");
    leakRateLabel->setTextInteractionFlags(Qt::NoTextInteraction);

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
    memoryLeaksLayout->setRowStretch(1, 3);
}

void MainWindow::updateGeneralMetrics(const GeneralMetrics &metrics)
{
    qDebug() << "=== ACTUALIZANDO INTERFAZ ===";
    qDebug() << "Current:" << metrics.currentUsageMB;
    qDebug() << "Active:" << metrics.activeAllocations;
    qDebug() << "Leaks:" << metrics.memoryLeaksMB;
    qDebug() << "Max:" << metrics.maxMemoryMB;
    qDebug() << "Total:" << metrics.totalAllocations;

    // Actualizar labels
    currentMemoryLabel->setText(QString("Uso actual: %1 MB").arg(metrics.currentUsageMB, 0, 'f', 2));
    activeAllocationsLabel->setText(QString("Asignaciones activas: %1").arg(metrics.activeAllocations));
    memoryLeaksLabel->setText(QString("Memory leaks: %1 MB").arg(metrics.memoryLeaksMB, 0, 'f', 2));
    maxMemoryLabel->setText(QString("Uso máximo: %1 MB").arg(metrics.maxMemoryMB, 0, 'f', 2));
    totalAllocationsLabel->setText(QString("Total asignaciones: %1").arg(metrics.totalAllocations));

    // Forzar actualización de la interfaz
    update();
    repaint();
}
