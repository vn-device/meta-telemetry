#include <QCoreApplication>
#include "MediaController.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    MediaController daemon(8081);
    return app.exec();
}