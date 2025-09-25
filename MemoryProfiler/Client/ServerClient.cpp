#include "ServerClient.h"

Client::Client(QObject *parent) : QObject(parent)
{
    socket = new QTcpSocket(this);
    clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    connect(socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &Client::onError);
}

bool Client::connectToServer(const QString &host, quint16 port)
{
    if (isConnected())
    {
        qDebug() << "Client: Ya conectado al servidor.";
        return true;
    }

    socket->connectToHost(host, port);
    return socket->waitForConnected(3000);
}

void Client::disconnectFromServer()
{
    if (socket)
    {
        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState)
        {
            socket->waitForDisconnected(1000);
        }
    }
}

void Client::send(const QString &keyword, const QByteArray &data)
{
    if (!isConnected())
    {
        qDebug() << "Client: No conectado, no se puede enviar:" << keyword;
        return;
    }

    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    QByteArray keywordBytes = keyword.toUtf8();

    // Formato compatible con Listen_Logic: [keyword_len][data_len][keyword][data]
    stream << quint16(keywordBytes.size());
    stream << quint16(data.size());
    packet.append(keywordBytes);
    packet.append(data);

    socket->write(packet);
    socket->flush();

    qDebug() << "Client: Enviado" << keyword << "size:" << data.size();
}

void Client::sendAlloc(quint64 address, quint64 size)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << address << size;

    send("ALLOC", data);
}

void Client::sendFree(quint64 address)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << address;

    send("FREE", data);
}

void Client::onConnected()
{
    qDebug() << "Client: Conectado al servidor. ID:" << clientId;
    emit connected();
}

void Client::onDisconnected()
{
    qDebug() << "Client: Desconectado del servidor.";
    emit disconnected();
}

void Client::onError(QAbstractSocket::SocketError error)
{
    QString errorStr = socket->errorString();
    qDebug() << "Client: Error de socket:" << errorStr;
    emit errorOccurred(errorStr);
}