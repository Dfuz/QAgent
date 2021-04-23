#include <QCoreApplication>
#include "qagent.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QAgent agent;
    agent.readConfig();
    agent.startAgent();

    return a.exec();
}
