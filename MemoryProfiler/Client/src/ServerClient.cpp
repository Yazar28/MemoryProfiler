#include "Client.h"

Client::Client(QObject *parent) : QObject(parent) {
    socket = new QTcpSocket(this);

    // Generamos un ID único con QUuid
    clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Conectar señales del socket
    connect(socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &Client::onError);
}

bool Client::connectToServer(const QString& host, quint16 port) {
    if (socket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "Ya conectado al servidor.";
        return true;
    }

    socket->connectToHost(host, port);

    if (!socket->waitForConnected(3000)) { // espera 3 segundos
        qDebug() << "Error de conexión:" << socket->errorString();
        return false;
    }

    return true;
}

void Client::send(const QString& key, const QByteArray& data) {
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "Cliente no conectado, no se puede enviar.";
        return;
    }

    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    QByteArray keyBytes = key.toUtf8();

    // Encabezado: longitudes
    stream << quint16(keyBytes.size());
    stream << quint16(data.size());

    // Cuerpo
    packet.append(keyBytes);
    packet.append(data);

    socket->write(packet);
    socket->flush();
}

void Client::onConnected() {
    qDebug() << "Cliente conectado al servidor.";
    emit connected();
}

void Client::onDisconnected() {
    qDebug() << "Cliente desconectado.";
    emit disconnected();
}

void Client::onError(QAbstractSocket::SocketError) {
    qDebug() << "Error de socket:" << socket->errorString();
    emit errorOccurred(socket->errorString());
}
