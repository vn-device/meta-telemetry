#include "TelemetryServer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QThread>
#include <utility>
#include <gpiod.hpp>
#include <map>
#include <fstream>
#include <string>
#include <iostream>
#include <array>
#include <memory>
#include <stdexcept>

// Translates physical header pins (1-40) to internal RP1 SoC line offsets
static int getBcmFromPhysical(int physicalPin) 
{
    static const std::map<int, int> pinMap = 
    {
        {3, 2}, {5, 3}, {7, 4}, {8, 14}, {10, 15}, {11, 17},
        {12, 18}, {13, 27}, {15, 22}, {16, 23}, {18, 24},
        {19, 10}, {21, 9}, {22, 25}, {23, 11}, {24, 8},
        {26, 7}, {29, 5}, {31, 6}, {32, 12}, {33, 13},
        {35, 19}, {36, 16}, {37, 26}, {38, 20}, {40, 21}
    };
    auto it = pinMap.find(physicalPin);
    return (it != pinMap.end()) ? it->second : -1;
}

TelemetryServer::TelemetryServer(quint16 port, QObject *parent) :
    QObject(parent),
    m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Telemetry Server"),
                                            QWebSocketServer::NonSecureMode, this))
{
    m_uptimeTimer.start();

    if (m_pWebSocketServer->listen(QHostAddress::Any, port)) 
    {
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

void TelemetryServer::onNewConnection() 
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();
    connect(pSocket, &QWebSocket::textMessageReceived, this, &TelemetryServer::processTextMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &TelemetryServer::socketDisconnected);
    m_clients << pSocket;
    qDebug() << "Client connected.";
}

void TelemetryServer::processTextMessage(const QString &message) 
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (!pClient) 
    {
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);
    
    if (error.error != QJsonParseError::NoError || !doc.isObject()) 
    {
        return;
    }

    QJsonObject root = doc.object();
    
    if (root["type"].toString() == "CLIENT_HELLO") 
    {
        QJsonObject response;
        response["type"] = "SYSTEM_READY";
        response["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        response["payload"] = QJsonObject(); 
        pClient->sendTextMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }
    
    if (root["type"].toString() == "WRITE_GPIO_REQUEST") 
    {
        QJsonObject payload = root["payload"].toObject();
        int physicalPin = payload["pin"].toInt();
        int val = payload["val"].toInt();
        QString mode = payload["mode"].toString();
        
        QJsonObject response;
        response["type"] = "COMMAND_RESPONSE";
        QJsonObject responsePayload;
        responsePayload["pin"] = physicalPin;
        
        int bcmPin = getBcmFromPhysical(physicalPin);
        
        if (bcmPin == -1) 
        {
            responsePayload["status"] = "ERROR";
            responsePayload["message"] = "Invalid or non-configurable physical pin requested.";
            response["payload"] = responsePayload;
            pClient->sendTextMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
            return;
        }
        
        try 
        {
            auto it = m_activeLines.find(physicalPin);
            
            if (it != m_activeLines.end()) 
            {
                m_activeLines.erase(it);
            }
            
            gpiod::chip chip("/dev/gpiochip0");
            gpiod::line_settings settings;
            
            if (mode == "OUT") 
            {
                settings.set_direction(gpiod::line::direction::OUTPUT);
                settings.set_output_value((val == 1) ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE);
            } 
            else 
            {
                settings.set_direction(gpiod::line::direction::INPUT);
            }
            
            auto req = chip.prepare_request()
                .set_consumer("telemetry-daemon")
                .add_line_settings(bcmPin, settings) 
                .do_request();
                
            m_activeLines.emplace(physicalPin, std::move(req));
            
            qDebug() << "Hardware GPIO BCM:" << bcmPin << "(Physical:" << physicalPin << ") mode:" << mode << "val:" << val;
            responsePayload["status"] = "SUCCESS";
        } 
        catch (const std::exception& e) 
        {
            qWarning() << "libgpiod error:" << e.what();
            responsePayload["status"] = "ERROR";
            responsePayload["message"] = QString("Hardware Lockout: ") + e.what();
        }
        
        response["payload"] = responsePayload;
        pClient->sendTextMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
}

void TelemetryServer::socketDisconnected() 
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient) 
    {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
        qDebug() << "Client disconnected.";
    }
}

qint64 TelemetryServer::getOsUptime() 
{
    QFile file("/proc/uptime");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) 
    {
        QString content = file.readAll();
        file.close();
        QStringList parts = content.split(" ");
        if (!parts.isEmpty()) 
        {
            return parts[0].toDouble();
        }
    }
    return 0;
}

float TelemetryServer::getCpuTemp() 
{
    QFile file("/sys/class/thermal/thermal_zone0/temp");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) 
    {
        QString content = file.readAll().trimmed();
        file.close();
        bool ok;
        float temp = content.toFloat(&ok);
        
        if (ok) 
        {
            return temp / 1000.0f; 
        }
    }

    return 0.0f;
}

float TelemetryServer::getLoadAvg() 
{
    QFile file("/proc/loadavg");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) 
    {
        QString content = file.readAll();
        file.close();

        QStringList parts = content.split(" ");
        if (!parts.isEmpty()) 
        {
            return parts[0].toFloat();
        }
    }

    return 0.0f;
}

int TelemetryServer::getRamUsage()
{
    std::ifstream file("/proc/meminfo");
    
    if (!file.is_open())
    {
        std::cerr << "Error: Unable to open /proc/meminfo" << std::endl;
        return -1;
    }

    std::string token;
    long memTotal = 0;
    long memAvailable = 0;

    while (file >> token)
    {
        if (token == "MemTotal:")
        {
            file >> memTotal;
        }
        else if (token == "MemAvailable:")
        {
            file >> memAvailable;
        }
        
        if (memTotal > 0 && memAvailable > 0)
        {
            break;
        }
    }

    file.close();

    if (memTotal > 0)
    {
        long usedMem = memTotal - memAvailable;
        return static_cast<int>((usedMem * 100) / memTotal);
    }

    return -1;
}

std::string TelemetryServer::executeCommand(const char* cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    
    // Leverage popen to fork a process and create a unidirectional pipe.
    // Wrap the FILE pointer in a unique_ptr with a custom deleter (pclose) 
    // to guarantee memory cleanup and prevent file descriptor leaks.
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    
    if (!pipe)
    {
        throw std::runtime_error("Failed to allocate pipe for command execution.");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    
    return result;
}

bool TelemetryServer::getUnderVoltageStatus()
{
    try
    {
        // The Broadcom VideoCore GPU returns a bitfield formatted as "throttled=0xXXXXX"
        std::string output = executeCommand("vcgencmd get_throttled");
        
        size_t pos = output.find("0x");
        if (pos != std::string::npos)
        {
            std::string hexStr = output.substr(pos + 2);
            uint32_t throttledState = std::stoul(hexStr, nullptr, 16);
            
            // Apply bitwise AND mask to extract specific hardware states:
            // Bit 0 (0x00001): Under-voltage currently occurring.
            return (throttledState & 0x01) != 0;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception during vcgencmd parse: " << e.what() << std::endl;
    }
    
    return false;
}

void TelemetryServer::broadcastHeartbeat() 
{
    QJsonObject payload;
    payload["os_uptime"] = getOsUptime();
    payload["daemon_uptime"] = m_uptimeTimer.elapsed() / 1000;
    payload["cpu_temp"] = getCpuTemp();
    payload["load_avg"] = getLoadAvg();
    payload["ram_usage"] = getRamUsage();
    payload["power_warn"] = getUnderVoltageStatus(); 

    QJsonObject packet;
    packet["type"] = "HEARTBEAT";
    packet["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    packet["payload"] = payload;

    QString msg = QJsonDocument(packet).toJson(QJsonDocument::Compact);
    for (QWebSocket *client : std::as_const(m_clients)) 
    {
        client->sendTextMessage(msg);
    }
}