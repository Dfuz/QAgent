#include "qagent.h"
#include "common/utils.h"

QAgent::QAgent(QObject *parent) : QObject(parent)
{
    initSocket();
    timer.callOnTimeout(this, &QAgent::startCollectData);

}

void QAgent::readConfig(QString settings_path)
{
    QSettings settings(settings_path, Utils::JsonFormat);

    settings_path = settings.fileName();

    if (!settings.value("ListenPort").isNull())
        listenPort = settings.value("ListenPort").toUInt();

    if (!settings.value("ListenIP").isNull())
        listenIP = QHostAddress{settings.value("ListenIP").toString()};

    if (!settings.value("BufferSize").isNull())
        bufferSize = settings.value("BufferSize").toUInt();

    if (!settings.value("RefreshActiveChecks").isNull())
        refreshActiveChecks = duration_cast<seconds>
                (
                    Utils::parseTime(settings.value("RefreshActiveChecks")
                                     .toString()
                                     )
                );
}

bool QAgent::startListen()
{
    connect(&localServer, &QTcpServer::newConnection, this, &QAgent::performPassiveCheck);

    if (!localServer.listen(listenIP, listenPort))
    {
        qDebug() << "Unable to start listen";
        return false;
    }
    qDebug() << "Listening started";
    return true;
}

int QAgent::getCompression()
{
    return compression;
}

void QAgent::setCompression(int newCompress)
{
    compression = newCompress;
}

bool QAgent::performPassiveCheck()
{
    // TODO


    return false;
}

void QAgent::initSocket()
{
    auto tcpSocket = std::make_shared<QTcpSocket>();
    tcpSocket->connectToHost(serverIP, serverPort);
    query = std::make_unique<Utils::QueryBuilder>(tcpSocket);
}

void QAgent::startCollectData()
{

}

void QAgent::performHandshake(std::shared_ptr<Utils::QueryBuilder> _query)
{
    auto message = Utils::ServiceMessage{R"({"who":"agent"})"};
    auto msg = _query->makeQueryRead()
           .toGet<Utils::Service>()
           .toSend(message)
           .invoke();
    if (msg)
    {
        auto newCompression = msg->payload["compression"];
        QAgent::setCompression(newCompression.toInt());
    }
}
