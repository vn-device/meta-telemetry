#include <QCoreApplication>
#include "TelemetryServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Listen on port 8080. The UI will establish a connection to ws://<Pi_IP>:8080
    TelemetryServer daemon(8080);
    
    // Enter the Qt event loop. This blocks indefinitely, efficiently routing 
    // underlying Linux epoll system events to your connected C++ slots.
    return app.exec();
}