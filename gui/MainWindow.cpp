// uso de librerias y archivos externos
#include "mainwindow.h"
#include "ListenLogic.h"
#include "profiler_structures.h"

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
    connect(listenLogic, &ListenLogic::memoryEventReceived, this, &MainWindow::onMemoryEventReceived);
    connect(listenLogic, &ListenLogic::fileAllocationSummaryUpdated, this, &MainWindow::updateFileAllocationSummary);

    connect(listenLogic, &ListenLogic::leakSummaryUpdated, this, &MainWindow::updateLeakSummary);
    connect(listenLogic, &ListenLogic::leaksByFileUpdated, this, &MainWindow::updateLeaksByFile);
    connect(listenLogic, &ListenLogic::leakTimelineUpdated, this, &MainWindow::updateLeakTimeline);
    memoryEventsHistory.clear();
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

void MainWindow::addMemoryEvent(const MemoryEvent &event)
{
    memoryEvents.append(event);

    // Mantener límite máximo
    if (memoryEvents.size() > MAX_MEMORY_EVENTS)
    {
        memoryEvents.removeFirst();
    }

    qDebug() << "📝 Evento agregado - Tipo:" << event.event_type
             << "Addr: 0x" << QString::number(event.address, 16)
             << "Total eventos:" << memoryEvents.size();
}

void MainWindow::updateMemoryEventsTable()
{
    // Actualizar la tabla existente de Memory Map con eventos
    memoryMapTable->setRowCount(memoryEvents.size());

    for (int i = 0; i < memoryEvents.size(); ++i)
    {
        const auto &event = memoryEvents[i];

        // Crear items para cada celda
        memoryMapTable->setItem(i, 0, new QTableWidgetItem(QString("0x%1").arg(event.address, 16, 16, QChar('0'))));
        memoryMapTable->setItem(i, 1, new QTableWidgetItem(formatMemorySize(event.size)));
        memoryMapTable->setItem(i, 2, new QTableWidgetItem(event.type));
        memoryMapTable->setItem(i, 3, new QTableWidgetItem(event.event_type)); // Event type instead of state
        memoryMapTable->setItem(i, 4, new QTableWidgetItem(QString("%1:%2").arg(event.filename).arg(event.line)));

        // Aplicar colores según el tipo de evento
        colorTableRow(i, event.event_type);
    }

    // Scroll automático al final para ver los eventos más recientes
    memoryMapTable->scrollToBottom();

    qDebug() << "🔄 Tabla de eventos actualizada - Eventos:" << memoryEvents.size();
}
// Modificar colorTableRow para manejar tipos de evento
void MainWindow::onMemoryEventReceived(const MemoryEvent &event)
{
    addToMemoryEventsHistory(event);
    updateMemoryEventsHistoryTable();
}

void MainWindow::addToMemoryEventsHistory(const MemoryEvent &event)
{
    // AGREGAR AL HISTORIAL SIN ELIMINAR NUNCA (solo si excede el límite máximo)
    memoryEventsHistory.append(event);

    // Mantener límite máximo para evitar consumo excesivo de memoria
    if (memoryEventsHistory.size() > MAX_MEMORY_EVENTS_HISTORY)
    {
        memoryEventsHistory.removeFirst();
        qDebug() << "🗑️ Eliminado evento más antiguo del historial. Total:" << memoryEventsHistory.size();
    }

    qDebug() << "📝 Evento agregado al historial - Tipo:" << event.event_type
             << "Addr: 0x" << QString::number(event.address, 16)
             << "Total eventos en historial:" << memoryEventsHistory.size();
}

void MainWindow::updateMemoryEventsHistoryTable()
{
    qDebug() << "🔄 Actualizando tabla de historial de eventos - Eventos:" << memoryEventsHistory.size();

    // Configurar tabla para mostrar todo el historial
    memoryMapTable->setRowCount(memoryEventsHistory.size());

    for (int i = 0; i < memoryEventsHistory.size(); ++i)
    {
        const auto &event = memoryEventsHistory[i];

        // Crear items para cada celda
        memoryMapTable->setItem(i, 0, new QTableWidgetItem(QString("0x%1").arg(event.address, 16, 16, QChar('0'))));
        memoryMapTable->setItem(i, 1, new QTableWidgetItem(formatMemorySize(event.size)));
        memoryMapTable->setItem(i, 2, new QTableWidgetItem(event.type));
        memoryMapTable->setItem(i, 3, new QTableWidgetItem(event.event_type));
        memoryMapTable->setItem(i, 4, new QTableWidgetItem(QString("%1:%2").arg(event.filename).arg(event.line)));

        // Aplicar colores según el tipo de evento
        colorTableRow(i, event.event_type);
    }

    // Scroll automático al final para ver los eventos más recientes
    memoryMapTable->scrollToBottom();

    qDebug() << "✅ Tabla de historial actualizada - Filas:" << memoryMapTable->rowCount();
}
void MainWindow::updateMemoryMap(const QVector<MemoryMapTypes::BasicMemoryBlock> &blocks)
{
    qDebug() << "🔄 Bloques de memoria recibidos:" << blocks.size();

    // SOLO PARA DEBUG: Mostrar información pero no actualizar la tabla principal
    // La tabla principal ahora solo muestra el historial de eventos

    for (const auto &block : blocks)
    {
        qDebug() << "  📍 Bloque - Addr: 0x" << QString::number(block.address, 16)
                 << "Size:" << block.size << "State:" << block.state
                 << "File:" << block.filename << "Line:" << block.line;
    }

    // NO actualizamos la tabla aquí, solo mostramos en consola para debug
    qDebug() << "ℹ️ Información de bloques mostrada en consola (tabla muestra solo historial de eventos)";
}
void MainWindow::colorTableRow(int row, const QString &eventType)
{
    QColor rowColor;
    QColor textColor = Qt::black;

    if (eventType == "alloc")
    {
        rowColor = QColor(200, 255, 200); // Verde claro para asignaciones
    }
    else if (eventType == "free")
    {
        rowColor = QColor(255, 255, 200); // Amarillo claro para liberaciones
    }
    else if (eventType == "realloc")
    {
        rowColor = QColor(200, 200, 255); // Azul claro para reasignaciones
    }
    else if (eventType == "leak") // 🆕 CASO ESPECÍFICO PARA LEAKS
    {
        rowColor = QColor(255, 200, 200); // Rojo claro para memory leaks
        qDebug() << "🎨 Color ROJO para fila" << row << "- Evento: leak";
    }
    else
    {
        rowColor = Qt::white; // Blanco por defecto
    }

    // Aplicar color a todas las celdas de la fila
    for (int col = 0; col < memoryMapTable->columnCount(); ++col)
    {
        QTableWidgetItem *item = memoryMapTable->item(row, col);
        if (!item)
        {
            item = new QTableWidgetItem();
            memoryMapTable->setItem(row, col, item);
        }
        item->setBackground(rowColor);
        item->setForeground(textColor);
    }
}
void MainWindow::updateFileAllocationSummary(const QVector<FileAllocationSummary> &fileAllocs)
{
    qDebug() << "🔄 Actualizando asignación por archivo - Archivos:" << fileAllocs.size();

    // Limpiar datos anteriores
    allocationPieSeries->clear();
    allocationBarSeries->clear();
    allocationTable->setRowCount(0);

    if (fileAllocs.isEmpty())
    {
        // Mostrar estado vacío
        allocationChart->setTitle("Distribución de Memoria por Archivo - Sin datos");
        allocationBarChart->setTitle("Asignaciones por Archivo - Sin datos");
        return;
    }

    // Ordenar por memoria total (descendente)
    QVector<FileAllocationSummary> sortedAllocs = fileAllocs;
    std::sort(sortedAllocs.begin(), sortedAllocs.end(),
              [](const FileAllocationSummary &a, const FileAllocationSummary &b)
              {
                  return a.memoryMB > b.memoryMB;
              });

    // Limitar a máximo de archivos para mejor visualización
    int displayCount = qMin(sortedAllocs.size(), MAX_ALLOCATION_FILES);

    // Preparar datos para gráficos
    QBarSet *barSet = new QBarSet("Memoria (MB)");
    QStringList categories;
    double otherMemory = 0.0;
    int otherAllocations = 0;
    quint64 otherLeaks = 0;
    double otherLeakedMemory = 0.0;

    // Procesar archivos para mostrar
    for (int i = 0; i < sortedAllocs.size(); ++i)
    {
        const FileAllocationSummary &alloc = sortedAllocs[i];

        if (i < displayCount)
        {
            // Agregar al gráfico de pastel
            QPieSlice *slice = allocationPieSeries->append(alloc.filename, alloc.memoryMB);
            slice->setLabel(QString("%1\n%2 MB").arg(alloc.filename).arg(alloc.memoryMB, 0, 'f', 1));

            // Agregar al gráfico de barras
            *barSet << alloc.memoryMB;
            categories << (alloc.filename.length() > 15 ? alloc.filename.left(12) + "..." : alloc.filename);

            // Agregar a la tabla
            int row = allocationTable->rowCount();
            allocationTable->insertRow(row);

            allocationTable->setItem(row, 0, new QTableWidgetItem(alloc.filename));
            allocationTable->setItem(row, 1, new QTableWidgetItem(QString::number(alloc.allocationCount)));
            allocationTable->setItem(row, 2, new QTableWidgetItem(QString::number(alloc.memoryMB, 'f', 2)));
            allocationTable->setItem(row, 3, new QTableWidgetItem(QString::number(alloc.leakCount)));
            allocationTable->setItem(row, 4, new QTableWidgetItem(QString::number(alloc.leakedMemoryMB, 'f', 2)));

            // Colorear filas con leaks
            if (alloc.leakCount > 0)
            {
                for (int col = 0; col < allocationTable->columnCount(); ++col)
                {
                    QTableWidgetItem *item = allocationTable->item(row, col);
                    if (item)
                    {
                        item->setBackground(QColor(255, 230, 230)); // Rojo claro para leaks
                    }
                }
            }
        }
        else
        {
            // Agrupar el resto como "Otros"
            otherMemory += alloc.memoryMB;
            otherAllocations += alloc.allocationCount;
            otherLeaks += alloc.leakCount;
            otherLeakedMemory += alloc.leakedMemoryMB;
        }
    }

    // Agregar "Otros" si es necesario
    if (otherMemory > 0)
    {
        QPieSlice *otherSlice = allocationPieSeries->append("Otros archivos", otherMemory);
        otherSlice->setLabel(QString("Otros archivos\n%1 MB").arg(otherMemory, 0, 'f', 1));

        // Agregar "Otros" a la tabla
        int row = allocationTable->rowCount();
        allocationTable->insertRow(row);
        allocationTable->setItem(row, 0, new QTableWidgetItem("Otros archivos"));
        allocationTable->setItem(row, 1, new QTableWidgetItem(QString::number(otherAllocations)));
        allocationTable->setItem(row, 2, new QTableWidgetItem(QString::number(otherMemory, 'f', 2)));
        allocationTable->setItem(row, 3, new QTableWidgetItem(QString::number(otherLeaks)));
        allocationTable->setItem(row, 4, new QTableWidgetItem(QString::number(otherLeakedMemory, 'f', 2)));
    }

    // Configurar gráfico de barras
    if (!barSet->count() && sortedAllocs.size() > 0)
    {
        // Si no se agregaron barras pero hay datos, agregar una barra para el primer elemento
        const FileAllocationSummary &alloc = sortedAllocs[0];
        *barSet << alloc.memoryMB;
        categories << (alloc.filename.length() > 15 ? alloc.filename.left(12) + "..." : alloc.filename);
    }

    if (barSet->count() > 0)
    {
        allocationBarSeries->append(barSet);

        QBarCategoryAxis *axisX = qobject_cast<QBarCategoryAxis *>(allocationBarChart->axes(Qt::Horizontal).first());
        if (axisX)
        {
            axisX->clear();
            axisX->append(categories);
        }

        // Auto-ajustar eje Y
        QValueAxis *axisY = qobject_cast<QValueAxis *>(allocationBarChart->axes(Qt::Vertical).first());
        if (axisY)
        {
            double maxValue = 0;
            for (int i = 0; i < barSet->count(); ++i)
            {
                if (barSet->at(i) > maxValue)
                    maxValue = barSet->at(i);
            }
            axisY->setRange(0, maxValue * 1.1); // 10% de margen
        }
    }

    // Actualizar títulos
    double totalMemory = 0;
    quint64 totalAllocations = 0;
    quint64 totalLeaks = 0;

    for (const FileAllocationSummary &alloc : sortedAllocs)
    {
        totalMemory += alloc.memoryMB;
        totalAllocations += alloc.allocationCount;
        totalLeaks += alloc.leakCount;
    }

    allocationChart->setTitle(QString("Distribución de Memoria por Archivo\nTotal: %1 MB, %2 asignaciones, %3 leaks")
                                  .arg(totalMemory, 0, 'f', 1)
                                  .arg(totalAllocations)
                                  .arg(totalLeaks));

    allocationBarChart->setTitle(QString("Memoria por Archivo (MB)\nTotal: %1 MB en %2 archivos")
                                     .arg(totalMemory, 0, 'f', 1)
                                     .arg(sortedAllocs.size()));

    qDebug() << "✅ Asignación por archivo actualizada - Mostrando" << displayCount << "archivos de" << sortedAllocs.size();
}
void MainWindow::setupAllocationByFileTab()
{
    allocationByFileTab = new QWidget();
    allocationByFileLayout = new QGridLayout(allocationByFileTab);

    // Crear splitter principal
    allocationSplitter = new QSplitter(Qt::Vertical);

    // ==================== GRÁFICO SUPERIOR ====================
    QGroupBox *chartGroup = new QGroupBox("Distribución de Memoria por Archivo");
    QVBoxLayout *chartLayout = new QVBoxLayout();

    // Gráfico de pastel
    allocationPieSeries = new QPieSeries();
    allocationPieSeries->setHoleSize(0.35);
    allocationPieSeries->setPieSize(0.8);

    allocationChart = new QChart();
    allocationChart->addSeries(allocationPieSeries);
    allocationChart->setTitle("Distribución de Memoria por Archivo");
    allocationChart->setAnimationOptions(QChart::AllAnimations);
    allocationChart->legend()->setAlignment(Qt::AlignRight);
    allocationChart->legend()->setFont(QFont("Arial", 9));

    allocationChartView = new QChartView(allocationChart);
    allocationChartView->setRenderHint(QPainter::Antialiasing);
    allocationChartView->setMinimumHeight(300);

    chartLayout->addWidget(allocationChartView);
    chartGroup->setLayout(chartLayout);

    // ==================== GRÁFICO DE BARRAS ====================
    QGroupBox *barChartGroup = new QGroupBox("Asignaciones por Archivo (Barras)");
    QVBoxLayout *barChartLayout = new QVBoxLayout();

    allocationBarSeries = new QBarSeries();
    allocationBarSeries->setLabelsVisible(true);
    allocationBarSeries->setLabelsFormat("@valuePtr MB");
    allocationBarSeries->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);

    allocationBarChart = new QChart();
    allocationBarChart->addSeries(allocationBarSeries);
    allocationBarChart->setTitle("Memoria por Archivo (MB)");
    allocationBarChart->setAnimationOptions(QChart::AllAnimations);
    allocationBarChart->legend()->setVisible(false);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    allocationBarChart->addAxis(axisX, Qt::AlignBottom);
    allocationBarSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Memoria (MB)");
    axisY->setLabelFormat("%.1f");
    allocationBarChart->addAxis(axisY, Qt::AlignLeft);
    allocationBarSeries->attachAxis(axisY);

    QChartView *barChartView = new QChartView(allocationBarChart);
    barChartView->setRenderHint(QPainter::Antialiasing);
    barChartView->setMinimumHeight(250);

    barChartLayout->addWidget(barChartView);
    barChartGroup->setLayout(barChartLayout);

    // ==================== TABLA INFERIOR ====================
    QGroupBox *tableGroup = new QGroupBox("Detalles por Archivo");
    QVBoxLayout *tableLayout = new QVBoxLayout();

    allocationTable = new QTableWidget();
    allocationTable->setColumnCount(5);
    allocationTable->setHorizontalHeaderLabels({"Archivo", "Asignaciones", "Memoria Total (MB)", "Leaks", "Memoria Leakeada (MB)"});
    allocationTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    allocationTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    allocationTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    allocationTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    allocationTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    allocationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    allocationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    allocationTable->setAlternatingRowColors(true);
    allocationTable->setSortingEnabled(true);

    tableLayout->addWidget(allocationTable);
    tableGroup->setLayout(tableLayout);

    // Organizar en splitter
    allocationSplitter->addWidget(chartGroup);
    allocationSplitter->addWidget(barChartGroup);
    allocationSplitter->addWidget(tableGroup);
    allocationSplitter->setSizes({350, 300, 400});

    allocationByFileLayout->addWidget(allocationSplitter, 0, 0);

    // Estado inicial
    updateFileAllocationSummary(QVector<FileAllocationSummary>());
}
void MainWindow::setupMemoryLeaksTab()
{
    memoryLeaksTab = new QWidget();
    memoryLeaksLayout = new QGridLayout(memoryLeaksTab);

    // ==================== RESUMEN ====================
    QGroupBox *summaryGroup = new QGroupBox("Resumen de Memory Leaks");
    QGridLayout *summaryLayout = new QGridLayout();

    // Crear labels de resumen
    totalLeakedMemoryLabel = new QLabel("Total memoria fugada: 0 MB");
    biggestLeakLabel = new QLabel("Leak más grande: 0 MB");
    mostFrequentLeakFileLabel = new QLabel("Archivo con más leaks: Ninguno");
    leakRateLabel = new QLabel("Tasa de leaks: 0%");

    // Configurar estilo de labels
    QFont summaryFont = totalLeakedMemoryLabel->font();
    summaryFont.setPointSize(10);
    totalLeakedMemoryLabel->setFont(summaryFont);
    biggestLeakLabel->setFont(summaryFont);
    mostFrequentLeakFileLabel->setFont(summaryFont);
    leakRateLabel->setFont(summaryFont);

    // Organizar en layout
    summaryLayout->addWidget(totalLeakedMemoryLabel, 0, 0);
    summaryLayout->addWidget(biggestLeakLabel, 0, 1);
    summaryLayout->addWidget(mostFrequentLeakFileLabel, 1, 0);
    summaryLayout->addWidget(leakRateLabel, 1, 1);

    summaryGroup->setLayout(summaryLayout);

    // ==================== GRÁFICAS ====================
    QGroupBox *chartsGroup = new QGroupBox("Gráficos de Memory Leaks");
    QGridLayout *chartsLayout = new QGridLayout();

    // Gráfico de barras - Leaks por archivo
    leaksByFileSeries = new QBarSeries();
    leaksByFileChart = new QChart();
    leaksByFileChart->addSeries(leaksByFileSeries);
    leaksByFileChart->setTitle("Leaks por Archivo");
    leaksByFileChart->setAnimationOptions(QChart::SeriesAnimations);
    leaksByFileChart->setTheme(QChart::ChartThemeLight);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    leaksByFileChart->addAxis(axisX, Qt::AlignBottom);
    leaksByFileSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Memoria Fugada (MB)");
    leaksByFileChart->addAxis(axisY, Qt::AlignLeft);
    leaksByFileSeries->attachAxis(axisY);

    leaksByFileChartView = new QChartView(leaksByFileChart);
    leaksByFileChartView->setRenderHint(QPainter::Antialiasing);
    leaksByFileChartView->setMinimumSize(400, 300);

    // Gráfico de pie - Distribución de leaks
    leaksDistributionSeries = new QPieSeries();
    leaksDistributionChart = new QChart();
    leaksDistributionChart->addSeries(leaksDistributionSeries);
    leaksDistributionChart->setTitle("Distribución de Leaks por Archivo");
    leaksDistributionChart->setAnimationOptions(QChart::SeriesAnimations);
    leaksDistributionChart->setTheme(QChart::ChartThemeLight);
    leaksDistributionChart->legend()->setAlignment(Qt::AlignRight);

    leaksDistributionChartView = new QChartView(leaksDistributionChart);
    leaksDistributionChartView->setRenderHint(QPainter::Antialiasing);
    leaksDistributionChartView->setMinimumSize(400, 300);

    // Gráfico de línea - Timeline de leaks
    leaksTimelineSeries = new QLineSeries();
    leaksTimelineSeries->setName("Memoria Fugada (MB)");
    leaksTimelineChart = new QChart();
    leaksTimelineChart->addSeries(leaksTimelineSeries);
    leaksTimelineChart->setTitle("Línea de Tiempo de Leaks");
    leaksTimelineChart->setAnimationOptions(QChart::SeriesAnimations);
    leaksTimelineChart->setTheme(QChart::ChartThemeLight);

    QValueAxis *timelineAxisX = new QValueAxis();
    timelineAxisX->setTitleText("Tiempo (segundos)");
    leaksTimelineChart->addAxis(timelineAxisX, Qt::AlignBottom);
    leaksTimelineSeries->attachAxis(timelineAxisX);

    QValueAxis *timelineAxisY = new QValueAxis();
    timelineAxisY->setTitleText("Memoria Fugada (MB)");
    leaksTimelineChart->addAxis(timelineAxisY, Qt::AlignLeft);
    leaksTimelineSeries->attachAxis(timelineAxisY);

    leaksTimelineChartView = new QChartView(leaksTimelineChart);
    leaksTimelineChartView->setRenderHint(QPainter::Antialiasing);
    leaksTimelineChartView->setMinimumSize(800, 300);

    // Organizar gráficas en el layout
    chartsLayout->addWidget(leaksByFileChartView, 0, 0);
    chartsLayout->addWidget(leaksDistributionChartView, 0, 1);
    chartsLayout->addWidget(leaksTimelineChartView, 1, 0, 1, 2);

    chartsGroup->setLayout(chartsLayout);

    // Organizar en layout principal
    memoryLeaksLayout->addWidget(summaryGroup, 0, 0);
    memoryLeaksLayout->addWidget(chartsGroup, 1, 0);

    // Configurar proporciones
    memoryLeaksLayout->setRowStretch(1, 3);
}
void MainWindow::updateLeakSummary(const LeakSummary &summary)
{
    currentLeakSummary = summary;

    // Actualizar labels
    totalLeakedMemoryLabel->setText(QString("Total memoria fugada: %1 MB").arg(summary.totalLeakedMB, 0, 'f', 2));
    biggestLeakLabel->setText(QString("Leak más grande: %1 MB (%2)").arg(summary.biggestLeakMB, 0, 'f', 2).arg(summary.biggestLeakLocation));
    mostFrequentLeakFileLabel->setText(QString("Archivo con más leaks: %1").arg(summary.mostFrequentLeakFile));
    leakRateLabel->setText(QString("Tasa de leaks: %1%").arg(summary.leakRate, 0, 'f', 2));

    qDebug() << "🔄 Resumen de leaks actualizado - Total:" << summary.totalLeakedMB << "MB";
}

void MainWindow::updateLeaksByFile(const QVector<LeakByFile> &leaksByFile)
{
    currentLeaksByFile = leaksByFile;

    // Limpiar series anteriores
    leaksByFileSeries->clear();
    leaksDistributionSeries->clear();

    if (leaksByFile.isEmpty())
        return;

    // Preparar datos para gráfico de barras
    QBarSet *barSet = new QBarSet("Memoria Fugada");
    QStringList categories;

    // Encontrar el valor máximo para escalar colores
    double maxLeak = 0;
    for (const LeakByFile &leak : leaksByFile)
    {
        if (leak.leakedMB > maxLeak)
            maxLeak = leak.leakedMB;
    }

    for (const LeakByFile &leak : leaksByFile)
    {
        *barSet << leak.leakedMB;

        // Acortar nombres largos de archivos
        QString shortName = leak.filename;
        if (shortName.length() > 20)
            shortName = shortName.left(17) + "...";
        categories << shortName;

        // Agregar al gráfico de pie
        QPieSlice *slice = leaksDistributionSeries->append(leak.filename, leak.leakedMB);

        // Colorear según la cantidad (rojo para leaks grandes)
        double intensity = leak.leakedMB / maxLeak;
        if (intensity > 0.7)
            slice->setColor(QColor(255, 100, 100));
        else if (intensity > 0.4)
            slice->setColor(QColor(255, 150, 100));
        else
            slice->setColor(QColor(255, 200, 100));

        slice->setLabel(QString("%1\n%2 MB").arg(leak.filename).arg(leak.leakedMB, 0, 'f', 1));
    }

    leaksByFileSeries->append(barSet);

    // Configurar eje X para gráfico de barras
    QBarCategoryAxis *axisX = qobject_cast<QBarCategoryAxis *>(leaksByFileChart->axes(Qt::Horizontal).first());
    if (axisX)
    {
        axisX->clear();
        axisX->append(categories);
    }

    // Auto-ajustar eje Y
    QValueAxis *axisY = qobject_cast<QValueAxis *>(leaksByFileChart->axes(Qt::Vertical).first());
    if (axisY)
    {
        double maxValue = 0;
        for (const LeakByFile &leak : leaksByFile)
        {
            if (leak.leakedMB > maxValue)
                maxValue = leak.leakedMB;
        }
        axisY->setRange(0, maxValue * 1.1); // 10% de margen
    }

    qDebug() << "📊 Gráficos de leaks por archivo actualizados - Archivos:" << leaksByFile.size();
}

void MainWindow::updateLeakTimeline(const QVector<LeakTimelinePoint> &leakTimeline)
{
    leakTimelineData = leakTimeline;
    leaksTimelineSeries->clear();

    if (leakTimeline.isEmpty())
        return;

    // Agregar puntos a la serie
    for (const LeakTimelinePoint &point : leakTimeline)
    {
        qint64 secondsFromStart = (point.timestamp - startTime) / 1000;
        leaksTimelineSeries->append(secondsFromStart, point.leakedMB);
    }

    // Auto-ajustar ejes
    if (!leakTimeline.isEmpty())
    {
        double minTime = (leakTimeline.first().timestamp - startTime) / 1000.0;
        double maxTime = (leakTimeline.last().timestamp - startTime) / 1000.0;
        double maxLeak = 0;

        for (const LeakTimelinePoint &point : leakTimeline)
        {
            if (point.leakedMB > maxLeak)
                maxLeak = point.leakedMB;
        }

        QValueAxis *axisX = qobject_cast<QValueAxis *>(leaksTimelineChart->axes(Qt::Horizontal).first());
        QValueAxis *axisY = qobject_cast<QValueAxis *>(leaksTimelineChart->axes(Qt::Vertical).first());

        if (axisX && axisY)
        {
            axisX->setRange(qMax(0.0, minTime - 5), maxTime + 5); // 5 segundos de margen
            axisY->setRange(0, maxLeak * 1.1);                    // 10% de margen
        }
    }

    qDebug() << "📈 Línea de tiempo de leaks actualizada - Puntos:" << leakTimeline.size();
}