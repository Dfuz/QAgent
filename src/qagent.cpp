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
        listenIP = QHostAddress{settings.value("ListenIP").toString()};

    // если имя хоста не задано, то в качестве имени будет взят хэш от мак-адреса
    if (settings.value("HostName").isNull())
        hostName = QCryptographicHash::hash(macAddress.toUtf8(),
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
}

void QAgent::startAgent()
{
    timer.start(refreshActiveChecks);
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Active checks started!";

    if (listenIP != QHostAddress::Null)
    {
        startListen();
        qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
                 << "Passive checks started!";
    }
}

bool QAgent::startListen()
{
    localServer = std::make_unique<QTcpServer>(this);
    connect(localServer.get(), &QTcpServer::newConnection, this, &QAgent::performPassiveCheck);

    if (!localServer->listen(listenIP, listenPort))
    {
        qCritical() << QTime::currentTime().toString(Qt::ISODateWithMs)
                    << "Unable to start local server!" << Qt::flush;
        localServer.reset(nullptr);
        return false;
    }
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Listening started";
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
    timer.stop();

    // попытка подключиться к серверу
    openSocket();
    if (!query)
    {
        qCritical() << QTime::currentTime().toString(Qt::ISODateWithMs)
                    << "No connection to server" << Qt::flush;
        closeSocket();
        return false;
    }
    performHandshake(query);
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
                << "Connected to server";

    // подготовка сообщения
    updateVirtualIds(*dataArray);
    QJsonArray jsonArray;
    for (auto const& it : *dataArray)
        jsonArray.push_back(it.toJson());
    QVariantMap payload
    {
        std::pair{"request", "agent data"},
        std::pair{"data", jsonArray},
        std::pair{"clock", static_cast<int>(std::time(nullptr))}
    };
    auto message = Utils::DataMessage{payload};

    // отправка сообщения
    auto response = query->makeQuery()
                          .toGet<Utils::Service>()
                          .toSend(message)
                          .invoke();
    if (!response.has_value())
    {
        qWarning() << "Something went wrong!..";
        closeSocket();
        return false;
    }
    if (response->response == QString("success"))
        qDebug() << "Success response";
    else qWarning() << "Something went wrong! Server response: " << response->response << Qt::flush;

    closeSocket();
    timer.start(refreshActiveChecks);
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
                        << "Unable connect to server!" << Qt::flush;
    }
}

inline void QAgent::closeSocket()
{
    query.reset(nullptr);
}

void QAgent::startCollectData()
{
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Data collection started" << Qt::flush;
    auto dataStruct = OS_UTILS::OS_EVENTS::pullOSStatus(confBitMask);
    auto localArray = toCollVec(dataStruct);

    for (auto const& it : localArray)
    {
        if (dataArray->size() == dataArray->capacity())
        {
            performActiveCheck();
            dataArray->clear();
        }
        dataArray->push_back(it);
    }

    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Data collection ended" << Qt::flush;
}

inline void QAgent::updateVirtualIds(collVec &vec)
{
    quint16 counter = 1;
    for (auto& it : vec)
        it.virtualId = counter++;
}

void QAgent::performHandshake(std::unique_ptr<Utils::QueryBuilder>& _query)
{
    QVariantMap payload
    {
        {"who", "agent"},
        {"macaddress", Utils::getMacAddress()}
    };
    auto message = Utils::HandshakeMessage{payload};
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
    if ((Utils::DataTypes::FileSystem & confBitMask) == confBitMask)
    {
        localVec.push_back
                (
                    {
                        this->hostName,
                        QString("TotalFSSize"),
                        (quint64)status.TotalFSSize,
                    }
                );

        localVec.push_back
                (
                    {
                        this->hostName,
                        QString("FreeFSSize"),
                        (quint64)status.FreeFSSize,
                    }
                );

        localVec.push_back
                (
                    {
                        this->hostName,
                        QString("FSMountPointsCount"),
                        status.allFs.count(),
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
                    }
                );

        localVec.push_back
                (
                    {
                        this->hostName,
                        QString("MemoryFree"),
                        (quint64)status.MemoryFree,
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
                    }
                );

        localVec.push_back
                (
                    {
                        this->hostName,
                        QString("psCount"),
                        (quint64)status.psCount,
                    }
                );
    }
    return localVec;
}
