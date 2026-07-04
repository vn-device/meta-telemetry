#ifndef TELEMETRYSERVER_H
#define TELEMETRYSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QTimer>
#include <QList>
#include <QElapsedTimer>

class TelemetryServer : public QObject
{
    Q_OBJECT
public:
    explicit TelemetryServer(quint16 port, QObject *parent = nullptr);
    ~TelemetryServer();

private slots:
    void onNewConnection();
    void processTextMessage(const QString &message);
    void socketDisconnected();
    void broadcastHeartbeat();

private:
    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    QTimer *m_pHeartbeatTimer;
    
    // Track daemon runtime internally
    QElapsedTimer m_uptimeTimer;
    
    void sendSystemState(QWebSocket *client);
    
    // Helper to read Linux OS uptime
    qint64 getOsUptime();
};

#endif // TELEMETRYSERVER_H