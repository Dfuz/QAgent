#include <QCommandLineParser>
#include <QCoreApplication>
#include "qagent.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("QAgent");
    QCoreApplication::setApplicationVersion("1.0");

    QAgent agent;
    agent.readConfig();

    QCommandLineParser parser;
    parser.setApplicationDescription("Test helper");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({{"n", "name"}, "Set custom hostname", "value"});
    parser.process(app);
    if (!parser.value("name").isEmpty())
        agent.setHostName(parser.value("name"));

    agent.startAgent();

    return app.exec();
}
