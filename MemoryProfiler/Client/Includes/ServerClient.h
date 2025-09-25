#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QUuid>
#include <QDebug>

class Client : public QObject {
    Q_OBJECT

public:
    explicit Client(QObject *parent = nullptr);

    QString getId() const { return clientId; }

    // Conectar al servidor
    bool connectToServer(const QString& host, quint16 port);

    // Enviar información al servidor
    void send(const QString& key, const QByteArray& data);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& err);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError);

private:
    QTcpSocket *socket;
    QString clientId;
};
