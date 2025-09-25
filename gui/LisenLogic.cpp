#pragma once
#include <QString>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>

class Listen_Logic {
public:
    Listen_Logic() = default;

    // Método que ejecuta la lógica según la keyword
    void applyLogic(const QString& keyword, const QByteArray& data) {
        if (keyword == "ALLOC") {
            quint64 addr, size;
            QDataStream s(data);
            s.setByteOrder(QDataStream::BigEndian);
            s >> addr >> size;

            qDebug() << "[LOGIC] ALLOC en" << QString("0x%1").arg(addr,0,16) << "size:" << size;

            // Aquí puedes luego llamar a tus funciones específicas
            // handleAlloc(addr, size);

        } else if (keyword == "FREE") {
            quint64 addr;
            QDataStream s(data);
            s.setByteOrder(QDataStream::BigEndian);
            s >> addr;

            qDebug() << "[LOGIC] FREE en" << QString("0x%1").arg(addr,0,16);

            // handleFree(addr);

        } else {
            qDebug() << "[LOGIC] KEYWORD desconocida:" << keyword;
        }
    }
};
