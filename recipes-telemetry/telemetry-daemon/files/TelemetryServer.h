#ifndef TELEMETRYSERVER_H
#define TELEMETRYSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QList>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>

class TelemetryServer : public QObject {
    Q_OBJECT
public:
    explicit TelemetryServer(quint16 port, QObject *parent = nullptr);
    ~TelemetryServer() override;

private slots:
    void onNewConnection();
    void processTextMessage(const QString &message);
    void socketDisconnected();
    void broadcastTelemetry();

private:
    QWebSocketServer *m_server;
    QList<QWebSocket *> m_clients;
    QTimer *m_telemetryTimer;

    // Track active runtime parameters
    uint64_t m_osRuntime;
    uint64_t m_appRuntime;
    bool m_gpio14State;
};
#endif // TELEMETRYSERVER_H