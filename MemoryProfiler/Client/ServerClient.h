#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QDebug>
#include <QDateTime>

class Client : public QObject
{
    Q_OBJECT

public:
    explicit Client(QObject *parent = nullptr);
    ~Client();

    bool isConnected() const { return socket && socket->state() == QAbstractSocket::ConnectedState; }

    // Conectar al servidor
    bool connectToServer(const QString &host = "localhost", quint16 port = 8080);
    void disconnectFromServer();

    // Método genérico para enviar cualquier tipo de dato
    template <typename T>
    void send(const QString &keyword, const T &data)
    {
        if (!isConnected())
        {
            qDebug() << "Client: No conectado, no se puede enviar:" << keyword;
            return;
        }

        QByteArray byteArray;
        QDataStream stream(&byteArray, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        stream << data;

        sendSerialized(keyword, byteArray);

        qDebug() << "Client: ✓ Envío exitoso - Key:" << keyword << "| Tamaño datos:" << byteArray.size() << "bytes";
    }

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &err);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);

private:
    QTcpSocket *socket;

    // Método interno para enviar datos serializados
    void sendSerialized(const QString &keyword, const QByteArray &data);
};