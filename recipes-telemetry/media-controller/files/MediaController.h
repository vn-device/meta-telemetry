#pragma once

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QTimer>
#include <QList>

class MediaController : public QObject
{
    Q_OBJECT
public:
    explicit MediaController(quint16 port, QObject *parent = nullptr);
    virtual ~MediaController();

private slots:
    void onNewConnection();
    void processTextMessage(const QString &message);
    void socketDisconnected();
    void generateMockFrame();

private:
    void sendAcknowledge(QWebSocket *pClient, const QString &origCommand, const QString &status, const QString &message);
    void startPreviewStream(int width, int height, int fps);
    void stopPreviewStream();

    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    
    // Mock rendering pipeline
    QTimer *m_pFrameTimer;
    quint32 m_frameIndex;
    int m_targetFps;
};
