#include "MediaController.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDateTime>
#include <sys/mman.h>
#include <unistd.h>
#include <QImage>
#include <QBuffer>
#include <QFile>
#include <QMetaObject>
#include <libcamera/control_ids.h>

using namespace libcamera;

MediaController::MediaController(quint16 port, QObject *parent) :
    QObject(parent),
    m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Media Controller"), QWebSocketServer::NonSecureMode, this)),
    m_isStreaming(false),
    m_capturePending(false),
    m_frameIndex(0),
    m_lastFrameTime(0)
{
    if (m_pWebSocketServer->listen(QHostAddress::Any, port))
    {
        qDebug() << "[SERVER] MediaController active on port" << port;
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &MediaController::onNewConnection);
    }
    
    initializeCamera();
}

MediaController::~MediaController()
{
    stopPreviewStream();
    
    if (m_camera)
    {
        m_camera->release();
        m_camera.reset();
    }
    
    if (m_cameraManager)
    {
        m_cameraManager->stop();
    }
    
    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void MediaController::initializeCamera()
{
    m_cameraManager = std::make_unique<CameraManager>();
    m_cameraManager->start();
    
    if (m_cameraManager->cameras().empty())
    {
        qWarning() << "Hardware Error: No libcamera-compatible devices found on the CSI interface.";
        return;
    }
    
    m_camera = m_cameraManager->cameras().front();
    if (m_camera->acquire() != 0)
    {
        qWarning() << "Hardware Lockout: Failed to acquire exclusivity over" << QString::fromStdString(m_camera->id());
        m_camera.reset();
        return;
    }
    
    qDebug() << "Hardware ISP acquired:" << QString::fromStdString(m_camera->id());
}

void MediaController::startPreviewStream(int width, int height, int fps)
{
    if (!m_camera || m_isStreaming)
    {
        return;
    }
    
    std::unique_ptr<libcamera::CameraConfiguration> config = m_camera->generateConfiguration({libcamera::StreamRole::Viewfinder});
    libcamera::StreamConfiguration &streamConfig = config->at(0);
    
    streamConfig.size.width = width;
    streamConfig.size.height = height;
    
    streamConfig.pixelFormat = libcamera::formats::RGB888;
    
    if (config->validate() == libcamera::CameraConfiguration::Invalid)
    {
        qWarning() << "Camera stream configuration rejected by ISP.";
        return;
    }
    
    m_camera->configure(config.get());
    m_stream = streamConfig.stream();
    
    m_allocator = std::make_unique<libcamera::FrameBufferAllocator>(m_camera);
    m_allocator->allocate(m_stream);
    
    const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &buffers = m_allocator->buffers(m_stream);
    
    for (unsigned int i = 0; i < buffers.size(); ++i)
    {
        const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
        
        void *memory = mmap(NULL, buffer->planes()[0].length, PROT_READ | PROT_WRITE, MAP_SHARED, buffer->planes()[0].fd.get(), 0);
        
        if (memory == MAP_FAILED)
        {
            qWarning() << "Memory Allocation Error: Failed to memory map DMA buffer index" << i;
            continue;
        }

        m_mappedBuffers[i] = std::make_pair(memory, buffer->planes()[0].length);
        
        std::unique_ptr<libcamera::Request> request = m_camera->createRequest(i);
        request->addBuffer(m_stream, buffer.get());
        m_requests.push_back(std::move(request));
    }
    
    m_camera->requestCompleted.connect(this, &MediaController::requestComplete);
    
    int64_t frameTimeMicroseconds = 1000000 / fps;
    libcamera::ControlList controls;
    controls.set(libcamera::controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({ frameTimeMicroseconds, frameTimeMicroseconds }));
    
    m_camera->start(&controls);
    
    for (std::unique_ptr<libcamera::Request> &request : m_requests)
    {
        m_camera->queueRequest(request.get());
    }
    
    m_isStreaming = true;
    qDebug() << "Hardware preview stream initialized at" << fps << "FPS.";
}

void MediaController::stopPreviewStream()
{
    if (!m_isStreaming || !m_camera)
    {
        return;
    }
    
    m_camera->stop();
    m_camera->requestCompleted.disconnect(this, &MediaController::requestComplete);
    
    for (auto const& [index, mapped] : m_mappedBuffers)
    {
        munmap(mapped.first, mapped.second);
    }
    
    m_mappedBuffers.clear();
    m_requests.clear();
    m_allocator.reset();
    
    m_isStreaming = false;
    qDebug() << "Hardware preview stream halted.";
}

void MediaController::requestComplete(Request *request)
{
    if (request->status() == Request::RequestCancelled)
    {
        return;
    }
    
    const FrameBuffer *buffer = request->buffers().at(m_stream);
    int requestCookie = request->cookie();
    
    void *memory = m_mappedBuffers[requestCookie].first;
    unsigned int bytesused = buffer->metadata().planes()[0].bytesused;
    
    if (bytesused > 0 && !m_clients.isEmpty())
    {
        QImage image(static_cast<const uchar*>(memory), 
                     m_stream->configuration().size.width, 
                     m_stream->configuration().size.height, 
                     m_stream->configuration().stride, 
                     QImage::Format_RGB888);

        // Intercept the active frame and dump to disk if a capture request is queued
        if (m_capturePending)
        {
            QString filePath = QString("/tmp/capture_%1.jpg").arg(QDateTime::currentMSecsSinceEpoch());
            
            if (image.save(filePath, "JPEG", 100))
            {
                QFile file(filePath);
                qint64 fileSizeBytes = file.size();
                
                QJsonObject payloadObj;
                payloadObj["command"] = "CAPTURE_IMAGE_REQUEST";
                payloadObj["file_path"] = filePath;
                payloadObj["file_size_bytes"] = fileSizeBytes;
                
                QJsonObject responseObj;
                responseObj["type"] = "COMMAND_RESPONSE";
                responseObj["payload"] = payloadObj;
                
                QJsonDocument doc(responseObj);
                QString responseStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
                
                QMetaObject::invokeMethod(this, [this, responseStr]()
                {
                    for (QWebSocket *client : std::as_const(m_clients))
                    {
                        client->sendTextMessage(responseStr);
                    }
                }, Qt::QueuedConnection);
                
                qDebug() << "Hardware capture complete:" << filePath;
            }
            else
            {
                qWarning() << "Disk I/O Error: Failed to write JPEG payload to /tmp.";
            }
            
            m_capturePending = false;
        }

        QByteArray jpegData;
        QBuffer imgBuffer(&jpegData);
        imgBuffer.open(QIODevice::WriteOnly);
        
        if (!image.save(&imgBuffer, "JPEG", 75))
        {
            qWarning() << "Backend Fault: QImage failed to encode raw RGB888 buffer to JPEG. Frame dropped.";
        }
        else
        {
            QByteArray header;
            header.resize(8);
            
            quint32 currentFrameIndex = m_frameIndex++;
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            quint32 timeDelta = m_lastFrameTime > 0 ? (now - m_lastFrameTime) : 0;
            m_lastFrameTime = now;
            
            header[0] = (currentFrameIndex >> 24) & 0xFF;
            header[1] = (currentFrameIndex >> 16) & 0xFF;
            header[2] = (currentFrameIndex >> 8) & 0xFF;
            header[3] = currentFrameIndex & 0xFF;
            
            header[4] = (timeDelta >> 24) & 0xFF;
            header[5] = (timeDelta >> 16) & 0xFF;
            header[6] = (timeDelta >> 8) & 0xFF;
            header[7] = timeDelta & 0xFF;
            
            QByteArray frameData = header;
            frameData.append(jpegData);
            
            QMetaObject::invokeMethod(this, [this, frameData]()
            {
                for (QWebSocket *client : std::as_const(m_clients))
                {
                    client->sendBinaryMessage(frameData);
                }
            }, Qt::QueuedConnection);
        }
    }
    
    request->reuse(Request::ReuseBuffers);
    m_camera->queueRequest(request);
}

void MediaController::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();
    connect(pSocket, &QWebSocket::textMessageReceived, this, &MediaController::processTextMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &MediaController::socketDisconnected);
    m_clients << pSocket;
    qDebug() << "Client bound to Media Controller routing layer.";
}

void MediaController::processTextMessage(const QString &message)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);
    
    if (error.error != QJsonParseError::NoError || !doc.isObject())
    {
        return;
    }

    QJsonObject root = doc.object();
    QString type = root["type"].toString();
    QJsonObject payload = root["payload"].toObject();
    
    if (type == "START_PREVIEW_STREAM")
    {
        int width = payload["width"].toInt(640);
        int height = payload["height"].toInt(360);
        int fps = payload["fps"].toInt(15);
        startPreviewStream(width, height, fps);
    }
    else if (type == "STOP_PREVIEW_STREAM")
    {
        stopPreviewStream();
    }
    else if (type == "CAPTURE_IMAGE_REQUEST")
    {
        m_capturePending = true;
    }
}

void MediaController::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient)
    {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
    
    if (m_clients.isEmpty())
    {
        stopPreviewStream();
    }
}