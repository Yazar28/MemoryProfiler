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
    // Inicializar ListenLogic
    listenLogic = new ListenLogic(this);
    connect(listenLogic, &ListenLogic::generalMetricsUpdated,
            this, &MainWindow::updateGeneralMetrics);
    // Conectar las señales de ListenLogic
    connect(listenLogic, &ListenLogic::generalMetricsUpdated,
            this, &MainWindow::updateGeneralMetrics);
    connect(listenLogic, &ListenLogic::timelinePointUpdated, // <- AGREGAR ESTA
            this, &MainWindow::updateTimelineChart);
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
// Cierra el servidor y las conexiones de los clientes
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
// Procesa los datos recibidos, maneja el buffer y llama a ListenLogic
void MainWindow::processData(const QByteArray &data)
{ // Manejo del buffer para datos parciales
    static QByteArray buffer;
    buffer.append(data);
    // Usar QDataStream para facilitar la lectura de datos binarios
    QDataStream stream(&buffer, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::BigEndian); // Debe coincidir con el cliente
    // bucle para procesar múltiples mensajes en el buffer
    while (buffer.size() >= sizeof(quint16) + sizeof(quint32))
    {
        // Guardar la posición actual
        qint64 startPos = stream.device()->pos();
        // Leer las longitudes del keyword y los datos
        quint16 keywordLen;
        quint32 dataLen;
        // Leer usando operadores >> en lugar de readRawData
        stream >> keywordLen >> dataLen;
        if (stream.status() != QDataStream::Ok) // caso "Error al leer"
        {
            qDebug() << "✗ Error: No se pudieron leer las longitudes";
            buffer.clear();
            return;
        }
        // Mostrar las longitudes leídas
        qDebug() << "Longitud del keyword:" << keywordLen << "bytes";
        qDebug() << "Longitud de los datos:" << dataLen << "bytes";
        // Verificar si tenemos todos los datos necesarios
        quint64 totalNeeded = sizeof(keywordLen) + sizeof(dataLen) + keywordLen + dataLen;
        if (buffer.size() - startPos < totalNeeded) // caso "Datos incompletos"
        {
            // No tenemos todos los datos aún, restaurar posición y esperar más
            stream.device()->seek(startPos);
            buffer = buffer.mid(startPos);
            return;
        }
        // Leer el keyword
        QByteArray keywordBytes(keywordLen, 0);
        if (stream.readRawData(keywordBytes.data(), keywordLen) != keywordLen) // caso "Error al leer keyword"
        {
            qDebug() << "✗ Error: No se pudo leer el keyword";
            buffer.clear();
            return;
        }
        // Convertir a QString
        QString keyword = QString::fromUtf8(keywordBytes);
        qDebug() << "Keyword recibido:" << keyword;
        // Leer los datos
        QByteArray receivedData(dataLen, 0);
        if (stream.readRawData(receivedData.data(), dataLen) != dataLen) // caso "Error al leer datos"
        {
            qDebug() << "✗ Error: No se pudo leer los datos completos";
            buffer.clear();
            return;
        }
        // Mostrar tamaño de los datos recibidos
        qDebug() << "Datos recibidos:" << receivedData.size() << "bytes";
        // Procesar usando ListenLogic
        if (listenLogic) // caso "ListenLogic no inicializado"
        {
            listenLogic->processData(keyword, receivedData);
        }
        else // caso "Error: ListenLogic no está inicializado"
        {
            qDebug() << "✗ Error: ListenLogic no está inicializado";
        }
        // Eliminar los datos procesados del buffer
        qint64 currentPos = stream.device()->pos();
        buffer = buffer.mid(currentPos);

        // Reiniciar el stream con el buffer actualizado
        stream.device()->seek(0);
    }
    // Si queda algo en el buffer que no se pudo procesar, conservarlo
    statusBar()->showMessage("Datos procesados - Clientes: " +                   // este primer paso es para manejar el buffer y procesar los datos recibidos
                             QString::number(clients.size()) +                   // luego se muestra en la status bar
                             " - Última actualización: " +                       // la hora actual
                             QDateTime::currentDateTime().toString("hh:mm:ss")); // Mostrar hora actual
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
void MainWindow::setupMemoryMapTab()
{ // Configura la pestaña de mapa de memoria
    memoryMapTab = new QWidget();
    memoryMapLayout = new QGridLayout(memoryMapTab);
    // Tabla de mapa de memoria
    QGroupBox *memoryMapGroup = new QGroupBox("Mapa de Memoria");
    QVBoxLayout *groupLayout = new QVBoxLayout();
    // Crear la tabla
    memoryMapTable = new QTableWidget();
    memoryMapTable->setColumnCount(5);
    memoryMapTable->setHorizontalHeaderLabels({"Dirección", "Tamaño", "Tipo", "Estado", "Archivo"});
    memoryMapTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    memoryMapTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memoryMapTable->setRowCount(0);
    // Añadir al layout
    groupLayout->addWidget(memoryMapTable);           // añade la tabla al layout del grupo
    memoryMapGroup->setLayout(groupLayout);           // establece el layout del grupo
    memoryMapLayout->addWidget(memoryMapGroup, 0, 0); // añade el grupo al layout principal
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
    qDebug() << "📈 Timeline actualizada - Puntos:" << timelineData.size()
             << "Memoria actual:" << point.memoryMB << "MB";
}
