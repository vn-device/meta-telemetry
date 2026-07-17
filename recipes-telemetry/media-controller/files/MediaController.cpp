#include "MediaController.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
#include <QDataStream>
#include <QByteArray>

MediaController::MediaController(quint16 port, QObject *parent)
    : QObject(parent)
    , m_pWebSocketServer(new QWebSocketServer(QStringLiteral("MediaController Daemon"), QWebSocketServer::NonSecureMode, this))
    , m_pFrameTimer(new QTimer(this))
    , m_frameIndex(0)
    , m_targetFps(15)
{
    if (m_pWebSocketServer->listen(QHostAddress::Any, port))
    {
        qDebug().noquote() << "[SERVER] MediaController active on port" << port;
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &MediaController::onNewConnection);
    }
    else
    {
        qCritical().noquote() << "[SERVER] Port binding failure on port" << port;
    }

    connect(m_pFrameTimer, &QTimer::timeout, this, &MediaController::generateMockFrame);
}

MediaController::~MediaController()
{
    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void MediaController::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();
    connect(pSocket, &QWebSocket::textMessageReceived, this, &MediaController::processTextMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &MediaController::socketDisconnected);
    m_clients << pSocket;
}

void MediaController::processTextMessage(const QString &message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (!pClient)
    {
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        return;
    }

    QJsonObject rootObj = doc.object();
    QString type = rootObj.value(QStringLiteral("type")).toString();
    QJsonObject payload = rootObj.value(QStringLiteral("payload")).toObject();

    if (type == QLatin1String("START_PREVIEW_STREAM"))
    {
        int w = payload.value(QStringLiteral("width")).toInt(640);
        int h = payload.value(QStringLiteral("height")).toInt(360);
        int fps = payload.value(QStringLiteral("fps")).toInt(15);
        
        startPreviewStream(w, h, fps);
        sendAcknowledge(pClient, type, QStringLiteral("SUCCESS"), QStringLiteral("Preview stream initialized."));
    }
    else if (type == QLatin1String("STOP_PREVIEW_STREAM"))
    {
        stopPreviewStream();
        sendAcknowledge(pClient, type, QStringLiteral("SUCCESS"), QStringLiteral("Preview stream stopped."));
    }
    else if (type == QLatin1String("CAPTURE_IMAGE_REQUEST"))
    {
        sendAcknowledge(pClient, type, QStringLiteral("SUCCESS"), QStringLiteral("Mock image captured."));
    }
    else if (type == QLatin1String("START_RECORDING_REQUEST") || type == QLatin1String("STOP_RECORDING_REQUEST"))
    {
        sendAcknowledge(pClient, type, QStringLiteral("SUCCESS"), QStringLiteral("Mock recording state toggled."));
    }
}

void MediaController::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient)
    {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
        
        if (m_clients.isEmpty())
        {
            stopPreviewStream();
        }
    }
}

void MediaController::startPreviewStream(int width, int height, int fps)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
    
    m_targetFps = (fps > 0) ? fps : 15;
    m_pFrameTimer->start(1000 / m_targetFps);
}

void MediaController::stopPreviewStream()
{
    m_pFrameTimer->stop();
}

void MediaController::generateMockFrame()
{
    if (m_clients.isEmpty())
    {
        return;
    }

    // Minimal 1x1 black pixel JPEG to trigger browser img.onload without GUI dependencies
    static const unsigned char dummyJpeg[] = 
    {
        0xff, 0xd8, 0xff, 0xdb, 0x00, 0x43, 0x00, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02,
        0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x06, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x06, 0x06, 0x05,
        0x06, 0x09, 0x08, 0x0a, 0x0a, 0x09, 0x08, 0x09, 0x09, 0x0b, 0x0c, 0x0f, 0x0c, 0x0b, 0x0b, 0x0e,
        0x0b, 0x09, 0x09, 0x0d, 0x11, 0x0d, 0x0e, 0x0f, 0x10, 0x10, 0x11, 0x10, 0x0a, 0x0c, 0x12, 0x13,
        0x12, 0x10, 0x13, 0x0f, 0x10, 0x10, 0x10, 0xff, 0xc0, 0x00, 0x0b, 0x08, 0x00, 0x01, 0x00, 0x01,
        0x01, 0x01, 0x11, 0x00, 0xff, 0xc4, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc4, 0x00, 0x14, 0x10, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xda, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3f, 0x00, 0x37, 0xff, 0xd9
    };

    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint32 timeDelta = 1000 / m_targetFps;
    
    stream << m_frameIndex << timeDelta;
    
    payload.append(reinterpret_cast<const char*>(dummyJpeg), sizeof(dummyJpeg));

    for (QWebSocket *client : qAsConst(m_clients))
    {
        client->sendBinaryMessage(payload);
    }

    m_frameIndex++;
}

void MediaController::sendAcknowledge(QWebSocket *pClient, const QString &origCommand, const QString &status, const QString &message)
{
    QJsonObject responseObj;
    responseObj.insert(QStringLiteral("type"), QStringLiteral("COMMAND_RESPONSE"));
    responseObj.insert(QStringLiteral("timestamp"), QDateTime::currentMSecsSinceEpoch());

    QJsonObject payloadObj;
    payloadObj.insert(QStringLiteral("command"), origCommand);
    payloadObj.insert(QStringLiteral("status"), status);
    payloadObj.insert(QStringLiteral("message"), message);

    if (origCommand == QLatin1String("CAPTURE_IMAGE_REQUEST"))
    {
        payloadObj.insert(QStringLiteral("file_path"), QStringLiteral("/tmp/mock_capture.jpg"));
        payloadObj.insert(QStringLiteral("file_size_bytes"), 1048576);
    }
    else if (origCommand == QLatin1String("STOP_RECORDING_REQUEST"))
    {
        payloadObj.insert(QStringLiteral("file_path"), QStringLiteral("/tmp/mock_video.mp4"));
        payloadObj.insert(QStringLiteral("file_size_bytes"), 5242880);
    }
    
    responseObj.insert(QStringLiteral("payload"), payloadObj);
    QJsonDocument doc(responseObj);
    pClient->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}
