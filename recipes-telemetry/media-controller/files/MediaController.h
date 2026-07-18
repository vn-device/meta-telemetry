#ifndef MEDIACONTROLLER_H
#define MEDIACONTROLLER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QList>
#include <QByteArray>
#include <libcamera/libcamera.h>
#include <map>
#include <memory>

class MediaController : public QObject
{
    Q_OBJECT
public:
    explicit MediaController(quint16 port, QObject *parent = nullptr);
    ~MediaController();

private Q_SLOTS:
    void onNewConnection();
    void processTextMessage(const QString &message);
    void socketDisconnected();

private:
    void initializeCamera();
    void startPreviewStream(int width, int height, int fps);
    void stopPreviewStream();
    void requestComplete(libcamera::Request *request);

    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    
    std::unique_ptr<libcamera::CameraManager> m_cameraManager;
    std::shared_ptr<libcamera::Camera> m_camera;
    std::unique_ptr<libcamera::FrameBufferAllocator> m_allocator;
    libcamera::Stream *m_stream;
    
    // Maps dmabuf file descriptors to virtual memory addresses
    std::map<int, std::pair<void *, unsigned int>> m_mappedBuffers;
    std::vector<std::unique_ptr<libcamera::Request>> m_requests;

    bool m_isStreaming;
    bool m_capturePending;
    uint32_t m_frameIndex;
    qint64 m_lastFrameTime;
};

#endif // MEDIACONTROLLER_H