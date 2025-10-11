#pragma once
#include <QObject>
#include <QVector>
#include <QHash>
#include <QString>
#include <QPair>

class Client; // forward-declare del cliente TCP

// Librería modular SIN simulación interna.
// Todo el “demo” se hace fuera (tests o app) usando este API.
class MemoryProfiler : public QObject {
    Q_OBJECT
public:
    static MemoryProfiler& instance();

    explicit MemoryProfiler(QObject* parent = nullptr);

    // Conecta el cliente (socket) que ya tienes
    void setClient(Client* c);

    // Limpia todo el estado interno (útil cuando vas a cargar un lote)
    void resetState();

    // ===== API de INGESTA (eventos externos) =====
    void recordAlloc(quint64 address, quint64 size,
                     const QString& filename, int line,
                     const QString& typeDetail = QStringLiteral("alloc"));

    void recordFree(quint64 address, const QString& filename = QString(), int line = 0);

    void recordLeak(quint64 address, const QString& filename = QString(), int line = 0);

    // ===== API de ENVÍO de métricas/gráficos =====
    // Punto de timeline del uso de memoria (gráfico principal)
    void pushTimelinePoint(double memoryMB, quint64 timestampMs = 0);

    // Enviar “General Metrics” basado en tu current/max, y el resto desde nuestro estado
    void sendGeneralFromState(double currentUsageMB, double maxMemoryMB);

    // Enviar resumen por archivo
    void sendFileSummary();

    // Enviar resumen de leaks + leaks por archivo + leak timeline (histórico)
    void updateLeakData();

    // Enviar Top Files
    void sendTopFiles(size_t topN = 8);

    // Enviar un snapshot del mapa (estado actual de todos los bloques)
    void sendBasicMemoryMapSnapshot();

private:
    // ---- Cliente TCP hacia el GUI ----
    Client* client_ = nullptr;

    // ---- Estado interno de memoria ----
    struct BasicBlock {
        quint64 address = 0;
        quint64 size    = 0;
        QString type;      // "alloc"
        QString state;     // "active" | "freed" | "leak"
        QString filename;
        int     line = 0;
    };
    QVector<BasicBlock> blocks_;
    QHash<quint64, int> indexByAddr_;  // address -> index en blocks_

    // Contadores (se actualizan en record*)
    quint64 totalAllocs_  = 0;
    quint64 activeAllocs_ = 0;

    // Acumuladores para Top Files
    QHash<QString, quint64> topCounts_; // archivo -> #allocs
    QHash<QString, quint64> topBytes_;  // archivo -> bytes

    // Leak timeline (buffer interno sin structs del cliente)
    QVector<QPair<quint64,double>> leakTimelineHistory_; // (timestamp, totalLeakedMB)
    int maxTimelinePoints_ = 120;

    // ===== Helpers =====
    void sendMemoryEvent(const QString& eventType,
                         quint64 address, quint64 size,
                         const QString& filename, int line,
                         const QString& typedetail);
};