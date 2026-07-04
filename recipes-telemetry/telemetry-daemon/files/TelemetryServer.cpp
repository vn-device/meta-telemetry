#include "TelemetryServer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QFile>

TelemetryServer::TelemetryServer(quint16 port, QObject *parent) :
    QObject(parent),
    m_pWebSocketServer(
        new QWebSocketServer(
            QStringLiteral("Telemetry Server"),
            QWebSocketServer::NonSecureMode, this
        )
    )
{
    // Start tracking daemon uptime
    m_uptimeTimer.start();

    if (m_pWebSocketServer->listen(QHostAddress::Any, port)) {
        qDebug() << "Telemetry Server up and listening on port" << port;
        
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &TelemetryServer::onNewConnection);
                
        m_pHeartbeatTimer = new QTimer(this);
        connect(m_pHeartbeatTimer, &QTimer::timeout, this, &TelemetryServer::broadcastHeartbeat);
        m_pHeartbeatTimer->start(1000); 
    }
}

TelemetryServer::~TelemetryServer()
{
    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

qint64 TelemetryServer::getOsUptime()
{
    QFile file("/proc/uptime");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString line = file.readLine();
        // The first value in /proc/uptime is the total OS uptime in seconds
        return line.split(' ').first().toDouble();
    }
    return 0;
}

void TelemetryServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();
    
    connect(pSocket, &QWebSocket::textMessageReceived, this, &TelemetryServer::processTextMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &TelemetryServer::socketDisconnected);
    
    m_clients << pSocket;
    qDebug() << "Client connected:" << pSocket->peerAddress().toString();
}

void TelemetryServer::processTextMessage(const QString &message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (!pClient) return;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);
    
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Invalid JSON schema received:" << error.errorString();
        return;
    }

    QJsonObject root = doc.object();
    QString type = root["type"].toString();

    if (type == "CLIENT_HELLO") {
        sendSystemState(pClient);
    } 
    else if (type == "WRITE_GPIO_REQUEST") {
        QJsonObject payload = root["payload"].toObject();
        int pin = payload["pin"].toInt();
        
        QJsonObject responsePayload;
        responsePayload["status"] = "SUCCESS";
        responsePayload["pin"] = pin;
        
        QJsonObject response;
        response["type"] = "COMMAND_RESPONSE";
        response["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        response["payload"] = responsePayload;
        
        pClient->sendTextMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
}

void TelemetryServer::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient) {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
        qDebug() << "Client disconnected";
    }
}

void TelemetryServer::broadcastHeartbeat()
{
    // Inject runtime metrics into the heartbeat payload
    QJsonObject payload;
    payload["daemon_uptime"] = m_uptimeTimer.elapsed() / 1000;
    payload["os_uptime"] = getOsUptime();

    QJsonObject heartbeat;
    heartbeat["type"] = "HEARTBEAT";
    heartbeat["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    heartbeat["payload"] = payload; // Added payload
    
    QString message = QJsonDocument(heartbeat).toJson(QJsonDocument::Compact);
    
    for (QWebSocket *client : std::as_const(m_clients)) {
        client->sendTextMessage(message);
    }
}

void TelemetryServer::sendSystemState(QWebSocket *client)
{
    QJsonObject pins;
    QJsonObject pin8;
    pin8["mode"] = "OUT";
    pin8["val"] = 0;
    pins["8"] = pin8;

    QJsonObject payload;
    payload["pins"] = pins;

    QJsonObject report;
    report["type"] = "SYSTEM_STATE_REPORT";
    report["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    report["payload"] = payload;

    client->sendTextMessage(QJsonDocument(report).toJson(QJsonDocument::Compact));
}