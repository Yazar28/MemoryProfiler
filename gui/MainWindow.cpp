// uso de librerias y archivos externos
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
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
// Constructor de la clase MainWindow
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // instancia de ListenLogic
    listenLogic = new ListenLogic(this);

    // Mantener todas las conexiones...
    connect(listenLogic, &ListenLogic::generalMetricsUpdated, this, &MainWindow::updateGeneralMetrics);
    connect(listenLogic, &ListenLogic::timelinePointUpdated, this, &MainWindow::updateTimelineChart);
    connect(listenLogic, &ListenLogic::topFilesUpdated, this, &MainWindow::updateTopFile);
    connect(listenLogic, &ListenLogic::basicMemoryMapUpdated, this, &MainWindow::updateMemoryMap);
    connect(listenLogic, &ListenLogic::memoryStatsUpdated, this, &MainWindow::updateMemoryStats);

    // INICIALIZAR PUNTEROS DE GRÁFICOS
    timelineChart = nullptr;
    timelineSeries = nullptr;
    axisX = nullptr;
    axisY = nullptr;
    startTime = QDateTime::currentMSecsSinceEpoch();

    // Configurar servidor TCP
    tcpServer = new QTcpServer(this);                                                   // Crear instancia del servidor TCP
    connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::onNewConnection); // Conectar la señal de nueva conexión
    serverStarted = false;                                                              // Inicializar como false
    // Configurar ventana principal
    setWindowTitle("Memory Profiler Server");
    setMinimumSize(1200, 800);
    // Crear contenedor principal con pila de widgets
    mainContainer = new QStackedWidget(this);
    setCentralWidget(mainContainer);
    // Crear la pestaña de conexión
    setupConnectionTab(); // pestaña de inicio de conexion (muestra el estado del servidor y permite iniciar/detener)
    // Crear las pestañas principales
    setupOverviewTab();         // pestaña de vista general
    setupMemoryMapTab();        // pestaña de mapa de memoria
    setupAllocationByFileTab(); // pestaña de asignacion por archivo
    setupMemoryLeaksTab();      // pestaña de memory leaks
    ///</summary> configura las pestañas principales y las añade al contenedor principal
    /// vista general
    /// mapa de memoria
    /// asignacion por archivo
    /// memory leaks
    ///</summary>
    tabWidget = new QTabWidget();
    tabWidget->addTab(overviewTab, "Vista General");
    tabWidget->addTab(memoryMapTab, "Mapa de Memoria");
    tabWidget->addTab(allocationByFileTab, "Asignación por Archivo");
    tabWidget->addTab(memoryLeaksTab, "Memory Leaks");
    // Añadir ambos a la pila
    mainContainer->addWidget(connectionTab); // Índice 0
    mainContainer->addWidget(tabWidget);     // Índice 1
    // Mostrar solo la pestaña de conexión al inicio
    mainContainer->setCurrentIndex(0); // Mostrar la pestaña de conexión al inicio
}
// Destructor de la clase MainWindow

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
void MainWindow::processData(const QByteArray &data)
{
    static QByteArray buffer;
    buffer.append(data);

    while (!buffer.isEmpty())
    {
        QDataStream stream(buffer);
        stream.setByteOrder(QDataStream::BigEndian);

        // Verificar si tenemos suficiente data para leer longitudes
        if (buffer.size() < static_cast<int>(sizeof(quint16) + sizeof(quint32)))
        {
            return; // Esperar más datos
        }

        // Leer longitudes SIN avanzar el puntero del stream
        quint16 keywordLen;
        quint32 dataLen;

        QByteArray temp = buffer;
        QDataStream tempStream(temp);
        tempStream.setByteOrder(QDataStream::BigEndian);
        tempStream >> keywordLen >> dataLen;

        // Calcular tamaño total del mensaje
        quint64 totalNeeded = sizeof(keywordLen) + sizeof(dataLen) + keywordLen + dataLen;

        if (buffer.size() < totalNeeded)
        {
            return; // Mensaje incompleto
        }

        // Ahora leer del buffer real
        stream >> keywordLen >> dataLen;

        // Leer keyword
        QByteArray keywordBytes(keywordLen, 0);
        if (stream.readRawData(keywordBytes.data(), keywordLen) != keywordLen)
        {
            qDebug() << "✗ Error leyendo keyword";
            buffer.clear();
            return;
        }
        QString keyword = QString::fromUtf8(keywordBytes);

        // Leer datos
        QByteArray receivedData(dataLen, 0);
        if (stream.readRawData(receivedData.data(), dataLen) != dataLen)
        {
            qDebug() << "✗ Error leyendo datos";
            buffer.clear();
            return;
        }

        // Procesar con ListenLogic
        if (listenLogic)
        {
            listenLogic->processData(keyword, receivedData);
        }

        // Remover mensaje procesado del buffer
        buffer = buffer.mid(totalNeeded);
    }
}
// Configura la pestaña de conexión
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
// Maneja el clic en el botón de iniciar/detener servidor
void MainWindow::onStartServerClicked()
{
    if (tcpServer->isListening())
    {
        // Detener el servidor y limpiar clientes
        tcpServer->close();
        for (QTcpSocket *client : clients)
        {
            client->close();
            client->deleteLater();
        }
        clients.clear(); // Limpiar la lista de clientes
        // Actualizar interfaz
        startServerButton->setText("Iniciar Servidor");
        serverStatusLabel->setText("Servidor detenido");
        clientsConnectedLabel->setText("Clientes conectados: 0");
        portInput->setEnabled(true);
        // Marcar que el servidor está detenido
        serverStarted = false;
        // Volver a la pestaña de conexión
        mainContainer->setCurrentIndex(0);
    }
    else
    {
        // Iniciar el servidor
        bool ok;                                 //
        int port = portInput->text().toInt(&ok); // Convertir a entero

        if (!ok || port <= 0 || port > 65535) // Validar puerto
        {
            QMessageBox::warning(this, "Error", "Puerto no válido");
            return;
        }
        // Intentar iniciar el servidor
        if (tcpServer->listen(QHostAddress::Any, port))
        {
            startServerButton->setText("Detener Servidor");
            serverStatusLabel->setText("Servidor iniciado en puerto " + QString::number(port));
            clientsConnectedLabel->setText("Clientes conectados: 0");
            portInput->setEnabled(false);
            // Marcar que el servidor está iniciado
            serverStarted = true;
            // Ir directamente a las pestañas principales
            mainContainer->setCurrentIndex(1);
            // Mostrar en status bar
            qDebug() << "✓ Servidor iniciado en puerto" << port << "- Esperando conexiones...";
        }
        else // Error al iniciar
        {
            QMessageBox::warning(this, "Error", "No se pudo iniciar el servidor: " + tcpServer->errorString());
        }
    }
}
// Maneja nuevas conexiones entrantes
void MainWindow::onNewConnection()
{
    // Obtener el socket del nuevo cliente
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &MainWindow::onClientDisconnected);
    // Añadir a la lista de clientes
    clients.append(clientSocket);
    clientsConnectedLabel->setText("Clientes conectados: " + QString::number(clients.size()));
    // Mostrar en consola
    qDebug() << "✓ Nuevo cliente conectado. Total:" << clients.size();
    qDebug() << "  Dirección:" << clientSocket->peerAddress().toString();
    qDebug() << "  Puerto:" << clientSocket->peerPort();
    // Mostrar las pestañas principales si no estaban visibles
    if (serverStarted && mainContainer->currentIndex() == 0)
    {
        mainContainer->setCurrentIndex(1);
    }
    // Mostrar en status bar
    statusBar()->showMessage("Nuevo cliente conectado - Total: " + QString::number(clients.size()));
}
// Maneja la desconexión de un cliente
void MainWindow::onClientDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (clientSocket)
    { // Eliminar de la lista y limpiar
        clients.removeOne(clientSocket);
        clientSocket->deleteLater();
        clientsConnectedLabel->setText("Clientes conectados: " + QString::number(clients.size()));
        // Mostrar en consola
        qDebug() << "✗ Cliente desconectado. Clientes restantes:" << clients.size();
        // Mostrar en status bar
        if (clients.isEmpty())
        {
            statusBar()->showMessage("Todos los clientes desconectados - Servidor sigue escuchando");
        }
        else
        {
            statusBar()->showMessage("Cliente desconectado - Clientes activos: " + QString::number(clients.size()));
        }
        // IMPORTANTE: No volver a la pestaña de conexión
        // El servidor sigue activo y escuchando nuevas conexiones
    }
}
// Maneja datos entrantes de un cliente
void MainWindow::onReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (!clientSocket)
        return;
    // Leer todos los datos disponibles
    QByteArray data = clientSocket->readAll();
    // Opcional: Identificar de qué cliente vienen los datos
    QString clientInfo = clientSocket->peerAddress().toString() + ":" + QString::number(clientSocket->peerPort()); // IP:Puerto (El proceso es el siguiente; se lee todo lo que haya en el socket y se almacena en data luego se muestra en consola y se procesa llamando a processData que maneja el buffer y llama a ListenLogic)
    // Mostrar en consola
    qDebug() << "Datos recibidos de cliente:" << clientInfo;
    qDebug() << "Tamaño:" << data.size() << "bytes";
    // Procesar los datos recibidos
    processData(data);
}
// Actualiza las métricas generales en la pestaña de vista general
void MainWindow::setupOverviewTab()
{
    // Crear la pestaña y su layout
    overviewTab = new QWidget();
    overviewLayout = new QGridLayout(overviewTab);
    // Panel de Métricas Generales
    QGroupBox *metricsGroup = new QGroupBox("Métricas Generales");
    QGridLayout *metricsLayout = new QGridLayout();
    // setea las labels
    // Uso actual de memoria
    currentMemoryLabel = new QLabel("Uso actual: 0 MB");                    // etiqueta que muestra el uso actual de memoria
    currentMemoryLabel->setTextInteractionFlags(Qt::NoTextInteraction);     // evita que el usuario pueda seleccionar el texto
    activeAllocationsLabel = new QLabel("Asignaciones activas: 0");         // etiqueta que muestra el numero de asignaciones activas
    activeAllocationsLabel->setTextInteractionFlags(Qt::NoTextInteraction); // evita que el usuario pueda seleccionar el texto
    memoryLeaksLabel = new QLabel("Memory leaks: 0 MB");                    // etiqueta que muestra la cantidad de memory leaks
    memoryLeaksLabel->setTextInteractionFlags(Qt::NoTextInteraction);       // evita que el usuario pueda seleccionar el texto
    maxMemoryLabel = new QLabel("Uso máximo: 0 MB");                        // etiqueta que muestra el uso máximo de memoria
    maxMemoryLabel->setTextInteractionFlags(Qt::NoTextInteraction);         // evita que el usuario pueda seleccionar el texto
    totalAllocationsLabel = new QLabel("Total asignaciones: 0");            // etiqueta que muestra el total de asignaciones
    totalAllocationsLabel->setTextInteractionFlags(Qt::NoTextInteraction);  // evita que el usuario pueda seleccionar el texto
    // Añadir al layout
    metricsLayout->addWidget(currentMemoryLabel, 0, 0);          // lyout de 3 filas y 2 columnas para organizar las etiquetas
    metricsLayout->addWidget(activeAllocationsLabel, 0, 1);      // la primera fila tiene el uso actual y las asignaciones activas
    metricsLayout->addWidget(memoryLeaksLabel, 1, 0);            // la segunda fila tiene los memory leaks y el uso máximo
    metricsLayout->addWidget(maxMemoryLabel, 1, 1);              // la tercera fila tiene el total de asignaciones
    metricsLayout->addWidget(totalAllocationsLabel, 2, 0, 1, 2); // esta etiqueta ocupa las dos columnas
    // Configurar espaciado
    metricsGroup->setLayout(metricsLayout);
    // Línea de tiempo - INICIALIZACIÓN DEL GRÁFICO
    QGroupBox *timelineGroup = new QGroupBox("Línea de Tiempo - Uso de Memoria");
    QVBoxLayout *timelineLayout = new QVBoxLayout();
    // Crear el gráfico
    timelineChart = new QChart();
    timelineChart->setTitle("Uso de Memoria en Tiempo Real");
    timelineChart->setAnimationOptions(QChart::NoAnimation);

    // Crear la serie de datos en el linea de tiempo
    timelineSeries = new QLineSeries();
    timelineSeries->setName("Memoria (MB)");
    timelineSeries->setColor(QColor(0, 120, 215));
    timelineChart->addSeries(timelineSeries);
    // Configurar ejes 'X' e 'Y'
    axisX = new QValueAxis();
    axisX->setTitleText("Tiempo (segundos)");
    axisX->setLabelFormat("%.1f");
    axisX->setRange(0, 60);
    axisY = new QValueAxis();
    axisY->setTitleText("Memoria (MB)");
    axisY->setLabelFormat("%.1f");
    axisY->setRange(0, 500);
    // Añadir ejes al gráfico
    timelineChart->addAxis(axisX, Qt::AlignBottom);
    timelineChart->addAxis(axisY, Qt::AlignLeft);
    timelineSeries->attachAxis(axisX);
    timelineSeries->attachAxis(axisY);
    // Crear la vista del gráfico
    timelineChartView = new QChartView(timelineChart);
    timelineChartView->setRenderHint(QPainter::Antialiasing);
    timelineLayout->addWidget(timelineChartView);
    timelineGroup->setLayout(timelineLayout);
    // Inicializar tiempo de inicio
    startTime = QDateTime::currentMSecsSinceEpoch();
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
// Actualiza las métricas generales en la pestaña de vista general
void MainWindow::setupMemoryMapTab() // Configura la pestaña de mapa de memoria
{
    memoryMapTab = new QWidget();                    // crea el widget principal de la pestaña
    memoryMapLayout = new QGridLayout(memoryMapTab); // layout principal de la pestaña
    // Panel de Mapa de Memoria
    QGroupBox *memoryMapGroup = new QGroupBox("Mapa de Memoria");
    QVBoxLayout *groupLayout = new QVBoxLayout();
    // Crear tabla
    memoryMapTable = new QTableWidget();
    memoryMapTable->setColumnCount(5);
    memoryMapTable->setHorizontalHeaderLabels({"Dirección", "Tamaño", "Tipo", "Estado", "Archivo:Línea"});
    // Aplicar estilo
    setupMemoryMapTableStyle();
    groupLayout->addWidget(memoryMapTable);
    memoryMapGroup->setLayout(groupLayout);
    memoryMapLayout->addWidget(memoryMapGroup, 0, 0);
}
// Actualiza las métricas generales en la pestaña de vista general
void MainWindow::setupAllocationByFileTab()
{ // Configura la pestaña de asignación por archivo
    allocationByFileTab = new QWidget();
    allocationByFileLayout = new QGridLayout(allocationByFileTab);
    // Gráfico de asignaciones por archivo
    QGroupBox *chartGroup = new QGroupBox("Asignación de Memoria por Archivo");
    QVBoxLayout *chartLayout = new QVBoxLayout();               // layout vertical para el gráfico
    allocationChartView = new QChartView();                     // vista del gráfico
    allocationChartView->setRenderHint(QPainter::Antialiasing); // mejora la calidad del renderizado
    chartLayout->addWidget(allocationChartView);                //  añade la vista del gráfico al layout
    chartGroup->setLayout(chartLayout);                         // establece el layout del grupo
    // Tabla detallada
    QGroupBox *tableGroup = new QGroupBox("Detalles por Archivo");
    QVBoxLayout *tableLayout = new QVBoxLayout();                                            // layout vertical para la tabla
    allocationTable = new QTableWidget();                                                    // tabla para mostrar detalles de asignación por archivo
    allocationTable->setColumnCount(3);                                                      // 3 columnas: archivo, asignaciones, memoria
    allocationTable->setHorizontalHeaderLabels({"Archivo", "Asignaciones", "Memoria (MB)"}); // establece los encabezados de las columnas
    allocationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);         // ajusta el tamaño de las columnas para que llenen el espacio disponible
    allocationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);                     // deshabilita la edición directa de las celdas
    allocationTable->setRowCount(0);                                                         // inicia con 0 filas
    // añade la tabla al layout del grupo
    tableLayout->addWidget(allocationTable); // añade la tabla al layout del grupo
    tableGroup->setLayout(tableLayout);      // establece el layout del grupo

    // Organizar en splitter para redimensionamiento
    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(chartGroup); // añade el grupo del gráfico al splitter
    splitter->addWidget(tableGroup); // añade el grupo de la tabla al splitter
    splitter->setSizes({400, 200});  //  establece tamaños iniciales para las dos secciones del splitter
    // Añadir al layout principal
    allocationByFileLayout->addWidget(splitter, 0, 0);
}
// Actualiza las métricas generales en la pestaña de vista general
void MainWindow::setupMemoryLeaksTab()
{                                                        // Configura la pestaña de memory leaks
    memoryLeaksTab = new QWidget();                      // crea el widget principal de la pestaña
    memoryLeaksLayout = new QGridLayout(memoryLeaksTab); // layout principal de la pestaña
    // Panel de resumen
    QGroupBox *summaryGroup = new QGroupBox("Resumen de Memory Leaks");
    QGridLayout *summaryLayout = new QGridLayout(); // layout de 2 filas y 2 columnas para las métricas de resumen
    // setea las labels
    totalLeakedMemoryLabel = new QLabel("Total memoria fugada: 0 MB");
    totalLeakedMemoryLabel->setTextInteractionFlags(Qt::NoTextInteraction); // evita que el usuario pueda seleccionar el texto
    // Leak más grande
    biggestLeakLabel = new QLabel("Leak más grande: 0 MB (archivo.cpp:123)");
    biggestLeakLabel->setTextInteractionFlags(Qt::NoTextInteraction); // evita que el usuario pueda seleccionar el texto
    // Archivo con más leaks
    mostFrequentLeakFileLabel = new QLabel("Archivo con más leaks: archivo.cpp");
    mostFrequentLeakFileLabel->setTextInteractionFlags(Qt::NoTextInteraction); // evita que el usuario pueda seleccionar el texto
    // Tasa de leaks
    leakRateLabel = new QLabel("Tasa de leaks: 0%");
    leakRateLabel->setTextInteractionFlags(Qt::NoTextInteraction); // evita que el usuario pueda seleccionar el texto
    // Añadir al layout
    summaryLayout->addWidget(totalLeakedMemoryLabel, 0, 0);    // añade las etiquetas al layout en la posición especificada
    summaryLayout->addWidget(biggestLeakLabel, 0, 1);          // añade las etiquetas al layout en la posición especificada
    summaryLayout->addWidget(mostFrequentLeakFileLabel, 1, 0); // añade las etiquetas al layout en la posición especificada
    summaryLayout->addWidget(leakRateLabel, 1, 1);             // añade las etiquetas al layout en la posición especificada
    // establece el layout del grupo
    summaryGroup->setLayout(summaryLayout); // establece el layout del grupo
    // Gráficos
    QGroupBox *chartsGroup = new QGroupBox("Gráficos de Memory Leaks");
    QGridLayout *chartsLayout = new QGridLayout(); // layout de 2 filas y 2 columnas para los gráficos
    leaksByFileChartView = new QChartView();
    leaksByFileChartView->setRenderHint(QPainter::Antialiasing); // mejora la calidad del renderizado
    leaksDistributionChartView = new QChartView();
    leaksDistributionChartView->setRenderHint(QPainter::Antialiasing); // mejora la calidad del renderizado
    leaksTimelineChartView = new QChartView();
    leaksTimelineChartView->setRenderHint(QPainter::Antialiasing); // mejora la calidad del renderizado
    // Añadir al layout
    chartsLayout->addWidget(leaksByFileChartView, 0, 0);         // añade la vista del gráfico al layout
    chartsLayout->addWidget(leaksDistributionChartView, 0, 1);   // añade la vista del gráfico al layout
    chartsLayout->addWidget(leaksTimelineChartView, 1, 0, 1, 2); // añade la vista del gráfico al layout, ocupando 2 columnas
    // establece el layout del grupo
    chartsGroup->setLayout(chartsLayout);
    // Organizar en el layout principal
    memoryLeaksLayout->addWidget(summaryGroup, 0, 0);
    memoryLeaksLayout->addWidget(chartsGroup, 1, 0);
    // Configurar proporciones
    memoryLeaksLayout->setRowStretch(1, 3);
}
// Actualiza las métricas generales en la pestaña de vista general
void MainWindow::updateGeneralMetrics(const GeneralMetrics &metrics)
{ // Actualiza las métricas generales en la pestaña de vista general
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
// Actualiza las métricas generales en la pestaña de vista general
void MainWindow::updateTimelineChart(const TimelinePoint &point)
{
    // Agregar el nuevo punto a los datos
    timelineData.append(point);
    // Limitar el número de puntos para no sobrecargar la UI
    if (timelineData.size() > MAX_POINTS) // CASO "Más de 300 puntos"
    {
        timelineData.removeFirst();
    }
    // Calcular tiempo relativo desde el inicio
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 elapsedSeconds = (currentTime - startTime) / 1000.0;
    // Actualizar la serie de datos
    timelineSeries->clear();
    double minMemory = 0; // valor mínimo de memoria
    double maxMemory = 0; // valor máximo de memoria

    if (!timelineData.isEmpty()) // caso "No hay datos"
    {
        minMemory = timelineData.first().memoryMB;
        maxMemory = timelineData.first().memoryMB;
    }
    for (const TimelinePoint &dataPoint : timelineData) // Iteracion para recorre todos los puntos de datos en timelineData
    {
        // Calcular tiempo relativo para este punto
        qint64 pointTime = (dataPoint.timestamp - startTime) / 1000.0;
        timelineSeries->append(pointTime, dataPoint.memoryMB);
        // Actualizar min/max
        if (dataPoint.memoryMB < minMemory) // caso "Nuevo mínimo"
            minMemory = dataPoint.memoryMB;
        if (dataPoint.memoryMB > maxMemory) // caso "Nuevo máximo"
            maxMemory = dataPoint.memoryMB;
    }
    // Auto-ajustar ejes con un pequeño margen
    if (timelineData.size() > 0) // caso "Hay datos para mostrar"
    {
        double memoryMargin = (maxMemory - minMemory) * 0.1; // 10% de margen

        axisY->setRange(
            qMax(0.0, minMemory - memoryMargin), // qmax es para evitar valores negativos en el eje Y de memoria
            maxMemory + memoryMargin);           // margen superior

        // El eje X siempre muestra los últimos 60 segundos
        // El eje X siempre muestra los últimos 60 segundos
        double startRange = qMax(0.0, (double)(elapsedSeconds - 60)); // Mostrar al menos desde 0 segundos
        double endRange = (double)elapsedSeconds;                     // Hasta el tiempo actual
        axisX->setRange(startRange, endRange);                        // Actualizar rango del eje X
    }
    // Forzar actualización del gráfico
    timelineChartView->update();
    // Mostrar en consola
    qDebug() << " Timeline actualizada - Puntos:" << timelineData.size()
             << "Memoria actual:" << point.memoryMB << "MB";
}
// Actualiza los archivos que mas asignaciones tuvieron en la pestaña de vista general
void MainWindow::updateTopFile(const QVector<TopFile> &topFiles) // Actualiza los top files en la pestaña de vista general
{                                                                // Limpiar la tabla
    topFilesTable->setRowCount(0);

    // Llenar con los top files (máximo 3)
    int row = 0;
    for (const TopFile &topFile : topFiles)
    {
        if (row >= 3)
            break; // Solo mostramos 3

        topFilesTable->insertRow(row);
        topFilesTable->setItem(row, 0, new QTableWidgetItem(topFile.filename));
        topFilesTable->setItem(row, 1, new QTableWidgetItem(QString::number(topFile.allocations)));
        topFilesTable->setItem(row, 2, new QTableWidgetItem(QString::number(topFile.memoryMB, 'f', 2)));
        row++;
    }

    // Forzar actualización
    topFilesTable->update();
    repaint();

    qDebug() << "Top Files actualizado - Filas:" << row;
}
//  Configurar estilo de la tabla
void MainWindow::setupMemoryMapTableStyle() // Configura el estilo y comportamiento de la tabla de mapa de memoria
{
    memoryMapTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memoryMapTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    memoryMapTable->setSelectionMode(QAbstractItemView::SingleSelection);
    memoryMapTable->setAlternatingRowColors(true);
    memoryMapTable->setSortingEnabled(true);

    // Scroll suave
    memoryMapTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    memoryMapTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    // Tamaño mínimo para scroll
    memoryMapTable->setMinimumHeight(400);

    // Configurar headers
    memoryMapTable->horizontalHeader()->setStretchLastSection(true);
    memoryMapTable->verticalHeader()->setDefaultSectionSize(25);

    // Columnas específicas
    memoryMapTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Dirección
    memoryMapTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Tamaño
    memoryMapTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Tipo
    memoryMapTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Estado
    memoryMapTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);          // Archivo
}

//  Actualizar mapa de memoria con bloques básicos
void MainWindow::updateMemoryMap(const QVector<MemoryMapTypes::BasicMemoryBlock> &blocks)
{
    qDebug() << "🔄 Actualizando Memory Map - Bloques recibidos:" << blocks.size();

    // AGREGAR CADA BLOQUE INDIVIDUAL AL HISTORIAL
    for (const auto &block : blocks)
    {
        addToMemoryMapHistory(block);
    }

    // ACTUALIZAR TABLA CON HISTORIAL COMPLETO
    memoryMapTable->setRowCount(memoryMapHistory.size());

    qDebug() << "📊 Llenando tabla con" << memoryMapHistory.size() << "filas";

    for (int i = 0; i < memoryMapHistory.size(); ++i)
    {
        const auto &block = memoryMapHistory[i];

        // CREAR ITEMS PARA CADA CELDA - ESTO ES CLAVE
        memoryMapTable->setItem(i, 0, new QTableWidgetItem(QString("0x%1").arg(block.address, 16, 16, QChar('0'))));
        memoryMapTable->setItem(i, 1, new QTableWidgetItem(formatMemorySize(block.size)));
        memoryMapTable->setItem(i, 2, new QTableWidgetItem(block.type));
        memoryMapTable->setItem(i, 3, new QTableWidgetItem(block.state));
        memoryMapTable->setItem(i, 4, new QTableWidgetItem(QString("%1:%2").arg(block.filename).arg(block.line)));

        // APLICAR COLORES - ESTO DEBE EJECUTARSE
        colorTableRow(i, block.state);
    }

    qDebug() << "✅ Tabla actualizada - Filas:" << memoryMapTable->rowCount();

    // FORZAR ACTUALIZACIÓN VISUAL
    memoryMapTable->viewport()->update();
    memoryMapTable->update();
}

//  Actualizar estadísticas de memoria
void MainWindow::updateMemoryStats(const MemoryMapTypes::MemoryStats &stats) // Actualiza las estadísticas de memoria en la barra de estado
{
    // Mostrar en barra de estado
    statusBar()->showMessage(
        QString("Memoria: %1 MB total | %2 MB activos | %3 MB leaks | Bloques: %4 total | %5 activos | %6 leaks")
            .arg(stats.totalMemoryMB, 0, 'f', 2)
            .arg(stats.activeMemoryMB, 0, 'f', 2)
            .arg(stats.leakedMemoryMB, 0, 'f', 2)
            .arg(stats.totalBlocks)
            .arg(stats.activeBlocks)
            .arg(stats.leakedBlocks));

    qDebug() << "📊 Stats actualizados - Activos:" << stats.activeBlocks
             << "Leaks:" << stats.leakedBlocks;
}
// Método auxiliar para formatear tamaño de memoria
QString MainWindow::formatMemorySize(quint64 bytes)
{
    if (bytes < 1024)
    {
        return QString("%1 bytes").arg(bytes);
    }
    else if (bytes < 1024 * 1024)
    {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 2);
    }
    else
    {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);
    }
}
//
void MainWindow::colorTableRow(int row, const QString &state)
{
    QColor rowColor;
    QColor textColor = Qt::black;

    if (state == "active")
    {
        rowColor = QColor(200, 230, 255); // Azul claro
        qDebug() << "🎨 Color AZUL para fila" << row << "- Estado: active";
    }
    else if (state == "freed")
    {
        rowColor = QColor(200, 255, 200); // Verde claro
        qDebug() << "🎨 Color VERDE para fila" << row << "- Estado: freed";
    }
    else if (state == "leak")
    {
        rowColor = QColor(255, 200, 200); // Rojo claro
        textColor = Qt::darkRed;
        qDebug() << "🎨 Color ROJO para fila" << row << "- Estado: leak";
    }
    else
    {
        rowColor = QColor(240, 240, 240); // Gris por defecto
    }

    // APLICAR COLOR A TODAS LAS CELDAS DE LA FILA
    for (int col = 0; col < memoryMapTable->columnCount(); ++col)
    {
        QTableWidgetItem *item = memoryMapTable->item(row, col);
        if (item)
        {
            item->setBackground(rowColor);
            item->setForeground(textColor);
        }
        else
        {
            qDebug() << "⚠️ Item nulo en fila" << row << "columna" << col;
        }
    }
}
//
void MainWindow::addToMemoryMapHistory(const MemoryMapTypes::BasicMemoryBlock &newBlock)
{
    // VERIFICAR SI EL BLOQUE YA EXISTE (POR DIRECCIÓN)
    bool exists = false;
    for (int i = 0; i < memoryMapHistory.size(); ++i)
    {
        if (memoryMapHistory[i].address == newBlock.address)
        {
            // ACTUALIZAR BLOQUE EXISTENTE
            memoryMapHistory[i] = newBlock;
            exists = true;
            qDebug() << "✏️ Actualizando bloque existente - Addr: 0x"
                     << QString::number(newBlock.address, 16);
            break;
        }
    }

    // SI ES NUEVO, AGREGARLO AL HISTORIAL
    if (!exists)
    {
        memoryMapHistory.append(newBlock);
        qDebug() << "➕ Nuevo bloque agregado - Addr: 0x"
                 << QString::number(newBlock.address, 16)
                 << "Estado:" << newBlock.state;
    }

    // MANTENER LÍMITE MÁXIMO
    if (memoryMapHistory.size() > MAX_MEMORY_MAP_HISTORY)
    {
        int removed = memoryMapHistory.size() - MAX_MEMORY_MAP_HISTORY;
        memoryMapHistory.remove(0, removed);
        qDebug() << "🗑️ Eliminados" << removed << "bloques antiguos del historial";
    }
}