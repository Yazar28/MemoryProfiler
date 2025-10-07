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
void Client::onError(QAbstractSocket::SocketError error)

{
    QString errorStr = socket->errorString();             // Obtener la descripción del error
    qDebug() << "Client: ✗ Error de socket:" << errorStr; // Mostrar el error
    emit errorOccurred(errorStr);                         // Emitir señal de error
}
void Client::sendGeneralMetrics(const GeneralMetrics &metrics)
{
    if (!isConnected())
    {
        qDebug() << "Client: No conectado, no se puede enviar GENERAL_METRICS";
        return;
    }
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << metrics;
    sendSerialized("GENERAL_METRICS", byteArray);
    qDebug() << "Client: ✓ GENERAL_METRICS enviado";
}

void Client::sendTimelinePoint(const TimelinePoint &point)
{
    if (!isConnected())
    {
        qDebug() << "Client: No conectado, no se puede enviar TIMELINE_POINT";
        return;
    }
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << point;
    sendSerialized("TIMELINE_POINT", byteArray);
    qDebug() << "Client: ✓ TIMELINE_POINT enviado";
}

void Client::sendTopFiles(const QVector<TopFile> &topFiles)
{
    if (!isConnected())
    {
        qDebug() << "Client: No conectado, no se puede enviar TOP_FILES";
        return;
    }
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << topFiles;
    sendSerialized("TOP_FILES", byteArray);
    qDebug() << "Client: ✓ TOP_FILES enviado";
}

void Client::sendBasicMemoryMap(const QVector<MemoryMapTypes::BasicMemoryBlock> &blocks)
{
    if (!isConnected())
    {
        qDebug() << "Client: No conectado, no se puede enviar BASIC_MEMORY_MAP";
        return;
    }
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << blocks;
    sendSerialized("BASIC_MEMORY_MAP", byteArray);
    qDebug() << "Client: ✓ BASIC_MEMORY_MAP enviado";
}

void Client::sendMemoryStats(const MemoryMapTypes::MemoryStats &stats)
{
    if (!isConnected())
    {
        qDebug() << "Client: No conectado, no se puede enviar MEMORY_STATS";
        return;
    }
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << stats;
    sendSerialized("MEMORY_STATS", byteArray);
    qDebug() << "Client: ✓ MEMORY_STATS enviado";
}
void Client::sendFileAllocationSummary(const QVector<FileAllocationSummary> &fileAllocs)
{
    if (!isConnected())
    {
        qDebug() << "Client: No conectado, no se puede enviar FILE_ALLOCATION_SUMMARY";
        return;
    }
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << fileAllocs;
    sendSerialized("FILE_ALLOCATION_SUMMARY", byteArray);
    qDebug() << "Client: ✓ FILE_ALLOCATION_SUMMARY enviado - Archivos:" << fileAllocs.size();
}
void Client::sendMemoryEvent(const MemoryEvent &event)
{
    if (!isConnected())
    {
        qDebug() << "Client: No conectado, no se puede enviar MEMORY_EVENT";
        return;
    }
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << event;
    sendSerialized("MEMORY_EVENT", byteArray);
    qDebug() << "Client: ✓ MEMORY_EVENT enviado";
}