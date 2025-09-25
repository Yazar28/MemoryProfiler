#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QUuid>
#include <QDebug>

class Client : public QObject
{
    Q_OBJECT

public:
    explicit Client(QObject *parent = nullptr);

    QString getId() const { return clientId; }
    bool isConnected() const { return socket && socket->state() == QAbstractSocket::ConnectedState; }

    // Conectar al servidor
    bool connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();

    // Enviar información al servidor (compatible con Listen_Logic)
    void send(const QString &keyword, const QByteArray &data);
    void sendAlloc(quint64 address, quint64 size);
    void sendFree(quint64 address);

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
    QString clientId;
};