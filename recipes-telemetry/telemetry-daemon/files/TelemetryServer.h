#ifndef TELEMETRYSERVER_H
#define TELEMETRYSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QTimer>
#include <QList>
#include <QElapsedTimer>
#include <gpiod.hpp> 
#include <map>

class TelemetryServer : public QObject {
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
    
    // Uptime tracking
    QElapsedTimer m_uptimeTimer;
    qint64 getOsUptime();
    
    // Hardware interface (libgpiod v2)
    std::map<int, gpiod::line_request> m_activeLines;
};

#endif // TELEMETRYSERVER_H