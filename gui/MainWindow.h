#ifndef MAINWINDOW_H // se usa para q no se incluya mas de una vez
#define MAINWINDOW_H // se usa para q no se incluya mas de una vez
// uso de librerias y modulos externos
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

class MainWindow : public QMainWindow // declaracion de la clase MainWindow que hereda de QMainWindow
{
Q_OBJECT                                                     // macro necesaria para usar señales y slots en Qt
                                                             // espacio publico de la clase
    public : explicit MainWindow(QWidget *parent = nullptr); // constructor de la clase MainWindow
    ~MainWindow();                                           // destructor de la clase MainWindow
    bool serverStarted;
    // variable para saber si el servidor esta iniciado
    // espacio privado los slots de la clase
private slots:
    void updateGeneralMetrics(const GeneralMetrics &metrics);
    void onStartServerClicked(); // slot para manejar el click en el boton de iniciar servidor
    void onNewConnection();      // slot para manejar una nueva conexion
    void onClientDisconnected(); // slot para manejar la desconexion de un cliente
    void onReadyRead();
    // parte privada de la clase
private:
    void setupConnectionTab();                            // configura la pestaña de conexion
    void setupOverviewTab();                              // configura la pestaña de overview
    void setupMemoryMapTab();                             // configura la pestaña de memory map
    void setupAllocationByFileTab();                      // configura la pestaña de allocation by file
    void setupMemoryLeaksTab();                           // configura la pestaña de memory leaks
    void processData(const QByteArray &data);             // procesa los datos recibidos
    void updateTimelineChart(const TimelinePoint &point); // actualiza el grafico de la linea de tiempo
    // Timeline Chart - AGREGAR ESTAS

    QChart *timelineChart;               // grafico de la linea de tiempo
    QLineSeries *timelineSeries;         //  serie de la linea de tiempo
    QValueAxis *axisX;                   // eje x de la linea de tiempo
    QValueAxis *axisY;                   // eje y de la linea de tiempo
    QVector<TimelinePoint> timelineData; // datos de la linea de tiempo
    qint64 startTime;                    // tiempo de inicio del programa
    const int MAX_POINTS = 100;          // maximo numero de puntos en el grafico
    // Server and networking
    QTcpServer *tcpServer;       // servidor TCP
    QList<QTcpSocket *> clients; // lista de clientes conectados
    bool hasClientEverConnected; // bandera para saber si algun cliente se ha conectado alguna vez
    ListenLogic *listenLogic;    // instancia de ListenLogic para manejar la logica de escucha y procesamiento de datos
    // Connection Tab
    QWidget *connectionTab;         // pestaña de conexion
    QLineEdit *portInput;           // input para el puerto
    QPushButton *startServerButton; // boton para iniciar el servidor
    QLabel *serverStatusLabel;      // label para mostrar el estado del servidor
    QLabel *clientsConnectedLabel;  // label para mostrar el numero de clientes conectados
    // Main tabs container
    QStackedWidget *mainContainer; // contenedor principal de las pestañas
    QTabWidget *tabWidget;         // widget de pestañas
    // Overview Tab poarte de vista general
    QWidget *overviewTab;           // pestaña de overview
    QGridLayout *overviewLayout;    // layout de la pestaña de overview
    QLabel *currentMemoryLabel;     // label para mostrar la memoria actual
    QLabel *activeAllocationsLabel; // label para mostrar las asignaciones activas
    QLabel *memoryLeaksLabel;       // label para mostrar las fugas de memoria
    QLabel *maxMemoryLabel;         // label para mostrar la memoria maxima
    QLabel *totalAllocationsLabel;  // label para mostrar las asignaciones totales
    QChartView *timelineChartView;  // vista del grafico de la linea de tiempo
    QTableWidget *topFilesTable;    // tabla para mostrar los archivos principales
    // Memory Map Tab parte del mapa de memoria
    QWidget *memoryMapTab;        // pestaña de memory map
    QGridLayout *memoryMapLayout; // layout de la pestaña de memory map
    QTableWidget *memoryMapTable; // tabla para mostrar el mapa de memoria
    // Allocation by File Tab parte de asignacion por archivo
    QWidget *allocationByFileTab;        // pestaña de allocation by file
    QGridLayout *allocationByFileLayout; // layout de la pestaña de allocation by file
    QChartView *allocationChartView;     // vista del grafico de asignacion por archivo
    QTableWidget *allocationTable;       // tabla para mostrar las asignaciones por archivo
    // Memory Leaks Tab parte de las fugas de memoria
    QWidget *memoryLeaksTab;                // pestaña de memory leaks
    QGridLayout *memoryLeaksLayout;         //
    QLabel *totalLeakedMemoryLabel;         // label para mostrar la memoria total filtrada
    QLabel *biggestLeakLabel;               // label para mostrar la mayor fuga
    QLabel *mostFrequentLeakFileabel;       // label para mostrar el archivo con mas fugas
    QLabel *leakRateLabel;                  // label para mostrar la tasa de fugas
    QLabel *mostFrequentLeakFileLabel;      // label para mostrar la tasa de fugas
    QChartView *leaksByFileChartView;       // vista del grafico de fugas por archivo
    QChartView *leaksDistributionChartView; // vista del grafico de distribucion de fugas
    QChartView *leaksTimelineChartView;     // vista del grafico de la linea de tiempo de fugas
};

#endif // end of MAINWINDOW_H sirve para evitar multiples inclusiones archivos que no lo utilizan
