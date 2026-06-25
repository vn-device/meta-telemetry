#include "TelemetryServer.h"
#include <sys/sysinfo.h>
#include <QDebug>

TelemetryServer::TelemetryServer(quint16 port, QObject *parent)
    : QObject(parent),
      m_server(new QWebSocketServer(QStringLiteral("Telemetry Daemon"), QWebSocketServer::NonSecureMode, this)),
      m_appRuntime(0),
      m_gpio14State(false)
{
    if (m_server->listen(QHostAddress::Any, port)) {
        qInfo() << "Telemetry WebSocket Server listening on port" << port;
        connect(m_server, &QWebSocketServer::newConnection, this, &TelemetryServer::onNewConnection);
    }
    else {
        qFatal("Failed to bind WebSocket server to port %d", port);
    }

    // Baseline broadcast loop (500ms).
    // For pure interrupt-driven architecture, this timer will be replaced
    // by QSocketNotifier watching the libgpiod file descriptor.
    m_telemetryTimer = new QTimer(this);
    connect(m_telemetryTimer, &QTimer::timeout, this, &TelemetryServer::broadcastTelemetry);
    m_telemetryTimer->start(500);
}

TelemetryServer::~TelemetryServer() {
    m_server->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void TelemetryServer::onNewConnection()
{
    QWebSocket *clientSocket = m_server->nextPendingConnection();
    
    // Connect socket signals to class slots to handle data and closures
    connect(clientSocket, &QWebSocket::textMessageReceived, this, &TelemetryServer::processTextMessage);
    connect(clientSocket, &QWebSocket::disconnected, this, &TelemetryServer::socketDisconnected);

    m_clients << clientSocket;
    qInfo() << "New telemetry client connected. Total clients:" << m_clients.size();
}

void TelemetryServer::processTextMessage(const QString &message)
{
    // Bi-directional IPC support. Allows the web dashboard to send commands directly to the hardware.
    if (message == "TOGGLE_GPIO_14") {
        m_gpio14State = !m_gpio14State;
        qInfo() << "Command received: Toggling GPIO 14 to" << (m_gpio14State ? "HIGH" : "LOW");
        broadcastTelemetry(); // Force immediate update
    }
}

void TelemetryServer::socketDisconnected()
{
    QWebSocket *clientSocket = qobject_cast<QWebSocket *>(sender());
    if (clientSocket) {
        m_clients.removeAll(clientSocket);
        
        // Critical: Queue object deletion in the Qt event loop to prevent memory leaks 
        // without crashing the active network handler.
        clientSocket->deleteLater(); 
        qInfo() << "Client disconnected. Remaining:" << m_clients.size();
    }
}

void TelemetryServer::broadcastTelemetry()
{
    if (m_clients.isEmpty()) return;

    // Retrieve POSIX kernel uptime
    struct sysinfo info;
    sysinfo(&info);
    m_osRuntime = info.uptime;
    m_appRuntime += 500; // Increment by timer interval (ms)

    // Construct the JSON telemetry payload
    QJsonObject payload;
    payload["os_runtime"] = static_cast<qint64>(m_osRuntime);
    payload["app_runtime"] = static_cast<qint64>(m_appRuntime / 1000);
    payload["gpio_14"] = m_gpio14State ? "HIGH" : "LOW";

    QJsonDocument doc(payload);
    QString jsonString = doc.toJson(QJsonDocument::Compact);

    // O(N) broadcast to all connected sink dashboards
    for (QWebSocket *client : std::as_const(m_clients)) {
        client->sendTextMessage(jsonString);
    }
}