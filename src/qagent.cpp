#include "qagent.h"
#include "common/utils.h"

QAgent::QAgent(QObject *parent) : QObject(parent)
{
    timer.callOnTimeout(this, &QAgent::startCollectData);
}

void QAgent::readConfig(QString settings_path)
{
    QSettings settings(settings_path, Utils::JsonFormat);

    settings_path = settings.fileName();

    // конфигурация для подключения к серверу
    if (!settings.value("ServerPort").isNull())
        serverPort = settings.value("ServerPort").toUInt();
    if (!settings.value("ServerIP").isNull())
        serverIP = QHostAddress{settings.value("ServerIP").toString()};

    // конфигурация для пассивных проверок
    if (!settings.value("ListenPort").isNull())
        listenPort = settings.value("ListenPort").toUInt();
    if (!settings.value("ListenIP").isNull())
    {
        listenIP = QHostAddress{settings.value("ListenIP").toString()};
        startListen();
    }

    // если имя хоста не задано, то в качестве имени будет взят хэш от мак-адреса
    if (settings.value("HostName").isNull())
        hostName = QCryptographicHash::hash(Utils::getMacAddress().toUtf8(),
                                            QCryptographicHash::Md4).toBase64();
    else hostName = settings.value("HostName").toString();


    if (!settings.value("BufferSize").isNull())
        bufferSize = settings.value("BufferSize").toUInt();
    dataArray->reserve(bufferSize);

    // "Configuration" : ["Memory", "Proccess", "FileSystem"],
    if (!settings.value("Configuration").isNull())
    {
        auto confMap = settings.value("Configuration").toList();
        for (const auto& now : confMap)
        {
            if (collectData.find(now.toString()) != end(collectData))
                confBitMask &= collectData.at(now.toString());
        }
    }

    if (!settings.value("RefreshActiveChecks").isNull())
        refreshActiveChecks = Utils::parseTime(settings.value("RefreshActiveChecks")
                                    .toString());
    timer.start(refreshActiveChecks);
}

bool QAgent::startListen()
{
    localServer = std::make_unique<QTcpServer>(this);
    connect(localServer.get(), &QTcpServer::newConnection, this, &QAgent::performPassiveCheck);

    if (!localServer->listen(listenIP, listenPort))
    {
        qCritical() << QTime::currentTime().toString(Qt::ISODateWithMs)
                    << "Unable to start local server!" << Qt::endl;
        localServer.reset(nullptr);
        return false;
    }
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Listening started" << Qt::endl;
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

bool QAgent::performActiveCheck()
{
    // попытка подключиться к серверу
    openSocket();
    if (!query)
    {
        qCritical() << QTime::currentTime().toString(Qt::ISODateWithMs)
                    << "No connection to server" << Qt::endl;
        return false;
    }
    performHandshake(query);
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
                << "Connected to server" << Qt::endl;

    // подготовка сообщения
    QJsonArray jsonArray;
    for (auto const& it : *dataArray)
        jsonArray.push_back(it.toJson());
    QVariantMap payload
    {
        std::pair{"request", "agent data"},
        std::pair{"data", jsonArray},
        std::pair{"clock", QString::number(std::time(nullptr))}
    };
    auto message = Utils::DataMessage{payload};
    auto response = query->makeQuery()
                          .toGet<Utils::Service>()
                          .toSend(message)
                          .invoke();
    if (!response.has_value())
    {
        qWarning() << "No response from server! Trying again..." << Qt::endl;
        auto response = query->makeQuery()
                              .toGet<Utils::Service>()
                              .toSend(message)
                              .invoke();
        if (!response.has_value())
            return false;
    }
    if (response->request == QString("success"))
        qDebug() << "Success response" << Qt::endl;

    closeSocket();
    return true;
}

inline void QAgent::openSocket()
{
    if (!serverIP.isNull() and serverPort)
    {
        auto tcpSocket = std::make_shared<QTcpSocket>();
        tcpSocket->connectToHost(serverIP, serverPort);
        if (tcpSocket->waitForConnected())
            query = std::make_unique<Utils::QueryBuilder>(tcpSocket);
        else qWarning() << QTime::currentTime().toString(Qt::ISODateWithMs)
                        << "Unable connect to server!" << Qt::endl;
    }
}

inline void QAgent::closeSocket()
{
    query.reset(nullptr);
}

void QAgent::startCollectData()
{
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Data collection started" << Qt::endl;
    auto dataStruct = OS_UTILS::OS_EVENTS::pullOSStatus(confBitMask);
    auto localArray = toCollVec(dataStruct);

    for (auto const& it : localArray)
    {
        if (dataArray->size() >= dataArray->capacity())
        {
            performActiveCheck();
            dataArray->clear();
        }
        dataArray->push_back(it);
    }

    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Data collection ended" << Qt::endl;
}

void QAgent::performHandshake(std::unique_ptr<Utils::QueryBuilder>& _query)
{
    auto message = Utils::HandshakeMessage{R"({"who":"agent"})"};
    auto msg = _query->makeQueryRead()
           .toGet<Utils::Handshake>()
           .toSend(message)
           .invoke();
    if (msg)
    {
        QAgent::setCompression(msg->compressionLevel);
    }
}

collVec QAgent::toCollVec(const OS_UTILS::OS_STATUS& status) const
{
    collVec localVec;
    auto counter = static_cast<quint16>(dataArray->size());
    if ((Utils::DataTypes::FileSystem & confBitMask) == confBitMask)
    {
        localVec.push_back
                (
                    {
                        this->hostName,
                        QString("TotalFSSize"),
                        (quint64)status.TotalFSSize,
                        counter++,
                    }
                );

        localVec.push_back
                (
                    {
                        this->hostName,
                        QString("FreeFSSize"),
                        (quint64)status.FreeFSSize,
                        counter++,
                    }
                );

        localVec.push_back
                (
                    {
                        this->hostName,
                        QString("FSMountPointsCount"),
                        status.allFs.count(),
                        counter++,
                    }
                );
    }

    if ((Utils::DataTypes::Memory & confBitMask) == confBitMask)
    {
        localVec.push_back
                (
                    {
                        this->hostName,
                        QString("MemoryTotal"),
                        (quint64)status.MemoryTotal,
                        counter++,
                    }
                );

        localVec.push_back
                (
                    {
                        this->hostName,
                        QString("MemoryFree"),
                        (quint64)status.MemoryFree,
                        counter++,
                    }
                );
    }

    if ((Utils::DataTypes::Process & confBitMask) == confBitMask)
    {
        localVec.push_back
                (
                    {
                        this->hostName,
                        QString("cpuLoad"),
                        (quint64)status.cpuLoad,
                        counter++,
                    }
                );

        localVec.push_back
                (
                    {
                        this->hostName,
                        QString("psCount"),
                        (quint64)status.psCount,
                        counter++,
                    }
                );
    }
    return localVec;
}
