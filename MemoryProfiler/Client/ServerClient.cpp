#include "ServerClient.h"

Client::Client(QObject *parent) : QObject(parent)
{
    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &Client::onError);

    qDebug() << "Client: Cliente inicializado correctamente";
}

Client::~Client()
{
    disconnectFromServer();
    delete socket;
}

bool Client::connectToServer(const QString &host, quint16 port)
{
    if (isConnected())
    {
        qDebug() << "Client: Ya estaba conectado al servidor";
        return true;
    }

    qDebug() << "Client: Conectando al servidor..." << host << ":" << port;
    socket->connectToHost(host, port);

    if (socket->waitForConnected(3000))
    {
        qDebug() << "Client: ✓ Conexión exitosa con el servidor";
        return true;
    }
    else
    {
        qDebug() << "Client: ✗ Error: No se pudo conectar al servidor";
        return false;
    }
}

void Client::disconnectFromServer()
{
    if (socket && isConnected())
    {
        qDebug() << "Client: Desconectando del servidor...";
        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState)
        {
            socket->waitForDisconnected(1000);
        }
        qDebug() << "Client: ✓ Desconectado del servidor";
    }
}

void Client::sendSerialized(const QString &keyword, const QByteArray &data)
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    QByteArray keywordBytes = keyword.toUtf8();

    // Formato: [keyword_len][data_len][keyword][data]
    stream << quint16(keywordBytes.size());
    stream << quint32(data.size());
    packet.append(keywordBytes);
    packet.append(data);

    qint64 bytesWritten = socket->write(packet);
    socket->flush();

    if (bytesWritten == packet.size())
    {
        qDebug() << "Client: ✓ Datos enviados correctamente al servidor";
    }
    else
    {
        qDebug() << "Client: ✗ Error: Solo se enviaron" << bytesWritten << "de" << packet.size() << "bytes";
    }
}

void Client::onConnected()
{
    qDebug() << "Client: ✓ Evento - Conexión establecida con servidor";
    emit connected();
}

void Client::onDisconnected()
{
    qDebug() << "Client: ✓ Evento - Desconectado del servidor";
    emit disconnected();
}

void Client::onError(QAbstractSocket::SocketError error)
{
    QString errorStr = socket->errorString();
    qDebug() << "Client: ✗ Error de socket:" << errorStr;
    emit errorOccurred(errorStr);
}