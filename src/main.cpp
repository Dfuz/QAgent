#include <QCommandLineParser>
#include <QCoreApplication>
#include "qagent.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("QAgent");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Test helper");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({{"n", "name"}, "Set custom hostname", "value"});
    parser.addOption({{"c", "config"}, "Set config file", "value"});
    parser.process(app);

    QAgent agent;
    if (parser.value("config").isEmpty())
        agent.readConfig();
    else agent.readConfig(parser.value("config"));

    if (!parser.value("name").isEmpty())
        agent.setHostName(parser.value("name"));

    agent.startAgent();

    return app.exec();
}
