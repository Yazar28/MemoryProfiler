#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QDebug>
#include <QDateTime>
#include <QString>
#include "profiler_structures.h"

class Client : public QObject
{
    Q_OBJECT

public:
    explicit Client(QObject *parent = nullptr); // Constructor
    ~Client();                                  // Destructor
    // Verificar si está conectado
    bool isConnected() const { return socket && socket->state() == QAbstractSocket::ConnectedState; }
    // Conectar al servidor
    bool connectToServer(const QString &host = "localhost", quint16 port = 8080); // Conectar al servidor
    void disconnectFromServer();                                                  // Desconectar del servidor
    // Eliminar el template y agregar estos métodos públicos:
    void sendGeneralMetrics(const GeneralMetrics &metrics);
    void sendTimelinePoint(const TimelinePoint &point);
    void sendTopFiles(const QVector<TopFile> &topFiles);
    void sendBasicMemoryMap(const QVector<MemoryMapTypes::BasicMemoryBlock> &blocks);
    void sendMemoryStats(const MemoryMapTypes::MemoryStats &stats);
    void sendMemoryEvent(const MemoryEvent &event);
    // En la clase Client en ServerClient.h, agregar:
    void sendFileAllocationSummary(const QVector<FileAllocationSummary> &fileAllocs);
signals:                                    // los singnals son señales que emite el objeto para notificar eventos
    void connected();                       // Emitida cuando se establece la conexión
    void disconnected();                    // Emitida cuando se pierde la conexión
    void errorOccurred(const QString &err); // Emitida cuando ocurre un error

private slots:                                        // los slots son funciones que responden a las señales
    void onConnected();                               // Ranura llamada cuando se establece la conexión
    void onDisconnected();                            // Ranura llamada cuando se pierde la conexión
    void onError(QAbstractSocket::SocketError error); // Ranura llamada cuando ocurre un errorS
private:
    QTcpSocket *socket; // Socket TCP para la comunicación
    // Método interno para enviar datos serializados
    void sendSerialized(const QString &keyword, const QByteArray &data); // sirve para enviar datos serializados al servidor
};
