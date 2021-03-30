#include "qagent.h"


QAgent::QAgent(QObject *parent) : QObject(parent)
{
    initSocket();
    timer.callOnTimeout(this, &QAgent::startSend);
}

int QAgent::getCompression()
{
    return compression;
}

void QAgent::setCompression(int newCompress)
{
    compression = newCompress;
}

void QAgent::initSocket()
{
    auto tcpSocket = std::make_shared<QTcpSocket>();
    tcpSocket->connectToHost(serverAddress, serverPort);
    query = std::make_unique<Utils::QueryBuilder>(tcpSocket);
}

void QAgent::startSend()
{

}

void QAgent::performHandshake(std::shared_ptr<Utils::QueryBuilder> _query)
{
    auto message = Utils::HandshakeMessage{R"({"who":"agent"})"};
    auto msg = _query->makeQueryRead()
           .toGet<Utils::Handshake>()
           .toSend(message)
           .invoke();
    
    return;
}
