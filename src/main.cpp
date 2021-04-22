#include <QCoreApplication>
#include "qagent.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QAgent agent;
    agent.readConfig();
    //agent.startListen();


    return a.exec();
}
