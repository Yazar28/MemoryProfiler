#include "ServerClient.h"

Client::Client(QObject *parent) : QObject(parent) // Constructor
{
    socket = new QTcpSocket(this); // Crear el socket TCP

    connect(socket, &QTcpSocket::connected, this, &Client::onConnected);       // Conectar señales y ranuras
    connect(socket, &QTcpSocket::disconnected, this, &Client::onDisconnected); // Conectar señales y ranuras
    connect(socket, &QTcpSocket::errorOccurred, this, &Client::onError);       // Conectar señales y ranuras

    qDebug() << "Client: Cliente inicializado correctamente"; // Mensaje de depuración
}
Client::~Client() // Destructor
{
    disconnectFromServer(); // Asegurarse de desconectar
    delete socket;          // Eliminar el socket
}
bool Client::connectToServer(const QString &host, quint16 port) // Conectar al servidor
{
    if (isConnected()) // caso ya esté conectado
    {
        qDebug() << "Client: Ya estaba conectado al servidor";
        return true;
    }
    // Intentar conectar
    qDebug() << "Client: Conectando al servidor..." << host << ":" << port;
    socket->connectToHost(host, port); // Intentar conectar

    if (socket->waitForConnected(5000)) // caso la conexión sea exitosa (esperar hasta 5 segundos)
    {
        qDebug() << "Client: ✓ Conexión exitosa con el servidor";
        return true;
    }
    else // caso la conexión falle
    {
        qDebug() << "Client: ✗ Error: No se pudo conectar al servidor";
        return false;
    }
}
void Client::disconnectFromServer() // Desconectar del servidor
{
    if (socket && isConnected()) // caso esté conectado
    {
        qDebug() << "Client: Desconectando del servidor...";
        socket->disconnectFromHost();                             // Intentar desconectar
        if (socket->state() != QAbstractSocket::UnconnectedState) // caso no se haya desconectado inmediatamente
        {
            socket->waitForDisconnected(3000); // Esperar hasta 3 segundo para desconectar
        }
        qDebug() << "Client: ✓ Desconectado del servidor";
    }
}
void Client::sendSerialized(const QString &keyword, const QByteArray &data) // Enviar datos serializados al servidor
{
    QByteArray packet;                                 // Paquete de datos a enviar
    QDataStream stream(&packet, QIODevice::WriteOnly); // Crear un flujo de datos para escribir en el paquete
    stream.setByteOrder(QDataStream::BigEndian);       // Usar orden de bytes Big Endian
    // Convertir la palabra clave a bytes UTF-8
    QByteArray keywordBytes = keyword.toUtf8(); // Convertir la palabra clave a bytes UTF-8
    // Formato: [keyword_len][data_len][keyword][data]
    stream << quint16(keywordBytes.size()); // Longitud de la palabra clave (2 bytes)
    stream << quint32(data.size());         // Longitud de los datos (4 bytes)
    packet.append(keywordBytes);            // Palabra clave
    packet.append(data);                    // Datos
    // Enviar el paquete completo
    // quint64 es un tipo de dato que se usa para representar un entero sin signo de 64 bits.
    qint64 bytesWritten = socket->write(packet); // Escribir en el socket
    socket->flush();                             // Asegurarse de que se envíe inmediatamente
    if (bytesWritten == packet.size())           // caso se hayan enviado todos los bytes
    {
        qDebug() << "Client: ✓ Datos enviados correctamente al servidor";
    }
    else // caso no se hayan enviado todos los bytes
    {
        qDebug() << "Client: ✗ Error: Solo se enviaron" << bytesWritten << "de" << packet.size() << "bytes";
    }
}
void Client::onConnected() // Ranura llamada cuando se establece la conexión
{
    qDebug() << "Client: ✓ Evento - Conexión establecida con servidor";
    emit connected(); // Emitir señal de conexión establecida
}
void Client::onDisconnected() // Ranura llamada cuando se pierde la conexión
{
    qDebug() << "Client: ✓ Evento - Desconectado del servidor";
    emit disconnected(); // Emitir señal de desconexión
}
void Client::onError(QAbstractSocket::SocketError error) //
{
    QString errorStr = socket->errorString();             // Obtener la descripción del error
    qDebug() << "Client: ✗ Error de socket:" << errorStr; // Mostrar el error
    emit errorOccurred(errorStr);                         // Emitir señal de error
}