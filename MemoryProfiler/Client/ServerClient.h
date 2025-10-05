#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QDebug>
#include <QDateTime>
#include <QString>

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
    // Un tamplete se usa para poder enviar cualquier tipo de dato(no recomendable si vas a operar con los datos solo para lectura)
    template <typename T>                            //
    void send(const QString &keyword, const T &data) // Enviar datos al servidor
    {
        if (!isConnected()) // caso no esté conectado
        {
            qDebug() << "Client: No conectado, no se puede enviar:" << keyword;
            return;
        }
        QByteArray byteArray;                                                                                        // Crear un QByteArray para serializar los datos
        QDataStream stream(&byteArray, QIODevice::WriteOnly);                                                        // Crear un flujo de datos para escribir en el QByteArray
        stream.setByteOrder(QDataStream::BigEndian);                                                                 // Usar orden de bytes Big Endian
        stream << data;                                                                                              // Serializar los datos en el flujo
        sendSerialized(keyword, byteArray);                                                                          // Enviar los datos serializados
        qDebug() << "Client: ✓ Envío exitoso - Key:" << keyword << "| Tamaño datos:" << byteArray.size() << "bytes"; // Mensaje de éxito
    }

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