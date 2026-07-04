#include <QCoreApplication>
#include "TelemetryServer.h"

int main(int argc, char *argv[])
{
    // QCoreApplication provides the event loop necessary for asynchronous networking
    QCoreApplication a(argc, argv);
    
    // Instantiate our WebSocket server on the port expected by the frontend
    TelemetryServer server(8080);
    
    // Start the event loop (this blocks and keeps the daemon running)
    return a.exec();
}