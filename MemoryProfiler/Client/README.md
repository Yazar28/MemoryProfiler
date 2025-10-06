````markdown
# 🧠 Guía de Implementación - Primer Tab (Vista General)

---

## 📘 Descripción General

Esta guía explica cómo **implementar el envío de datos desde la biblioteca del Memory Profiler** hacia el servidor principal.  
Incluye la configuración del cliente (`Client`), las estructuras necesarias, los intervalos de actualización y el ejemplo completo de uso.

---

## ⚙️ Estructuras ya funcionales en la interfaz

### Estructuras utilizadas

```cpp
GeneralMetrics    // Métricas mostradas en labels
TimelinePoint     // Puntos de la gráfica
TopFile           // Datos de la tabla Top 3
```
````

### Señales conectadas

```cpp
generalMetricsUpdated  → updateGeneralMetrics()
timelinePointUpdated   → updateTimelineChart()
topFilesUpdated        → updateTopFile()
```

### Elementos activos en la interfaz

- Labels de métricas generales
- Gráfica del timeline
- Tabla Top 3 de archivos con más asignaciones

---

## 🧩 Estructuras que debe enviar la biblioteca

```cpp
#include "profiler_structures.h"

// Estructuras requeridas para la vista general
GeneralMetrics metrics;
TimelinePoint timelinePoint;
QVector<TopFile> topFiles;
```

---

## 🔌 Configuración del Client y Conexión

### Instanciación y conexión del cliente

```cpp
#include "ServerClient.h"  // Biblioteca del cliente

class MemoryProfiler {
private:
    Client client;   // Instancia del cliente
    bool connected;

public:
    void initialize() {
        // Conectar al servidor
        connected = client.connectToServer("localhost", 8080);

        if (!connected) {
            qDebug() << "Error: No se pudo conectar al servidor";
            return;
        }

        qDebug() << "Conectado al servidor correctamente";

        // Iniciar envío de datos
        startSendingData();
    }
};
```

---

## 📤 Envío Correcto de Datos

### Ejemplo de envío para cada estructura

```cpp
void MemoryProfiler::sendGeneralMetrics() {
    if (!connected) return;

    GeneralMetrics metrics;
    // Llenar con datos reales...
    client.send("GENERAL_METRICS", metrics);
}

void MemoryProfiler::sendTimelinePoint() {
    if (!connected) return;

    TimelinePoint point;
    // Llenar con datos reales...
    client.send("TIMELINE_POINT", point);
}

void MemoryProfiler::sendTopFiles() {
    if (!connected) return;

    QVector<TopFile> topFiles;
    // Llenar con datos reales...
    client.send("TOP_FILES", topFiles);
}
```

---

## 🧠 Ejemplo Completo de Implementación

```cpp
#include "ServerClient.h"
#include "profiler_structures.h"
#include <QTimer>
#include <QDebug>

class MemoryProfiler : public QObject {
    Q_OBJECT

private:
    Client client;
    QTimer* metricsTimer;
    QTimer* timelineTimer;
    QTimer* topFilesTimer;
    bool isConnected;

public:
    MemoryProfiler(QObject* parent = nullptr) : QObject(parent) {
        isConnected = false;
        metricsTimer = new QTimer(this);
        timelineTimer = new QTimer(this);
        topFilesTimer = new QTimer(this);
    }

    bool startProfiling() {
        // Conexión al servidor
        isConnected = client.connectToServer("localhost", 8080);

        if (!isConnected) {
            qDebug() << "Error: No se pudo conectar al servidor";
            return false;
        }

        qDebug() << "Conectado al servidor correctamente";

        // Envíos periódicos
        connect(metricsTimer, &QTimer::timeout, this, &MemoryProfiler::sendGeneralMetrics);
        metricsTimer->start(2000);  // Cada 2 segundos

        connect(timelineTimer, &QTimer::timeout, this, &MemoryProfiler::sendTimelinePoint);
        timelineTimer->start(1000);  // Cada 1 segundo

        connect(topFilesTimer, &QTimer::timeout, this, &MemoryProfiler::sendTopFiles);
        topFilesTimer->start(8000);  // Cada 8 segundos

        return true;
    }

private slots:
    void sendGeneralMetrics() {
        if (!isConnected) return;

        GeneralMetrics metrics;
        metrics.currentUsageMB = getCurrentMemoryUsage();
        metrics.activeAllocations = getActiveAllocations();
        metrics.memoryLeaksMB = getMemoryLeaks();
        metrics.maxMemoryMB = getMaxMemoryUsage();
        metrics.totalAllocations = getTotalAllocations();

        client.send("GENERAL_METRICS", metrics);
        qDebug() << "Enviado GENERAL_METRICS";
    }

    void sendTimelinePoint() {
        if (!isConnected) return;

        TimelinePoint point;
        point.timestamp = QDateTime::currentMSecsSinceEpoch();
        point.memoryMB = getCurrentMemoryUsage();

        client.send("TIMELINE_POINT", point);
        qDebug() << "Enviado TIMELINE_POINT";
    }

    void sendTopFiles() {
        if (!isConnected) return;

        QVector<TopFile> topFiles = calculateTopFiles();
        client.send("TOP_FILES", topFiles);
        qDebug() << "Enviado TOP_FILES -" << topFiles.size() << "archivos";
    }

private:
    // Métodos que deben implementarse con la lógica real
    double getCurrentMemoryUsage() { /* ... */ }
    quint64 getActiveAllocations() { /* ... */ }
    double getMemoryLeaks() { /* ... */ }
    double getMaxMemoryUsage() { /* ... */ }
    quint64 getTotalAllocations() { /* ... */ }
    QVector<TopFile> calculateTopFiles() { /* ... */ }
};
```

---

## 🧪 Programa Principal de Prueba

```cpp
#include <QCoreApplication>
#include "MemoryProfiler.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    MemoryProfiler profiler;

    if (!profiler.startProfiling()) {
        qDebug() << "No se pudo iniciar el profiling";
        return -1;
    }

    qDebug() << "Memory Profiler iniciado - Enviando datos...";
    return app.exec();
}
```

---

## 🛠️ Configuración en CMake

```cmake
# CMakeLists.txt de la biblioteca del profiler
find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_library(MemoryProfilerLib STATIC
    MemoryProfiler.cpp
    # Otros archivos de implementación
)

target_link_libraries(MemoryProfilerLib
    PRIVATE
        ServerClient  # Biblioteca del cliente
        Qt6::Core
        Qt6::Network
)

# Programa de prueba
add_executable(TestProfiler test_main.cpp)
target_link_libraries(TestProfiler PRIVATE MemoryProfilerLib)
```

---

## 📈 Datos de Referencia para Pruebas

### GeneralMetrics

| Campo             | Rango esperado |
| ----------------- | -------------- |
| currentUsageMB    | 45.7 – 128.3   |
| activeAllocations | 500 – 2000     |
| memoryLeaksMB     | 0.0 – 15.2     |
| maxMemoryMB       | 150.0 – 300.0  |
| totalAllocations  | 5000 – 50000   |

### TimelinePoint

- `timestamp`: tiempo actual en milisegundos
- `memoryMB`: igual a `currentUsageMB`

### TopFiles (ejemplo)

```cpp
[0]: {filename: "renderer.cpp", allocations: 4500, memoryMB: 128.7}
[1]: {filename: "physics.cpp", allocations: 3200, memoryMB: 89.3}
[2]: {filename: "main.cpp", allocations: 1500, memoryMB: 45.5}
```

---

## 🧾 Serialización de Estructuras

Implementar los siguientes operadores para transmisión con `QDataStream`:

```cpp
QDataStream &operator<<(QDataStream &stream, const GeneralMetrics &metrics) {
    stream << metrics.currentUsageMB
           << metrics.activeAllocations
           << metrics.memoryLeaksMB
           << metrics.maxMemoryMB
           << metrics.totalAllocations;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const TimelinePoint &point) {
    stream << point.timestamp << point.memoryMB;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const QVector<TopFile> &topFiles) {
    stream << quint32(topFiles.size());
    for (const TopFile &file : topFiles) {
        stream << file.filename << file.allocations << file.memoryMB;
    }
    return stream;
}
```

---

## 🎯 Resumen Final

El desarrollador solo debe:

1. Incluir `ServerClient.h`.
2. Crear la instancia: `Client client;`.
3. Conectar con `client.connectToServer("localhost", 8080);`.
4. Enviar con `client.send("KEYWORD", estructura);`.
5. Usar las estructuras definidas en `profiler_structures.h`.

**Intervalos de envío recomendados:**

- `GENERAL_METRICS` → cada **2 segundos**
- `TIMELINE_POINT` → cada **1 segundo**
- `TOP_FILES` → cada **8 segundos**

---

> **Nota:**
> Este módulo no debe ejecutarse si no se utiliza activamente para el proyecto del profiler.
> El cliente descargará y enviará recursos específicos, por lo tanto, **no debe incluirse en repositorios públicos** ni ejecutarse fuera del entorno controlado de desarrollo.

```

```
