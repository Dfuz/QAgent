#include "qagent.h"
#include "common/utils.h"

QAgent::QAgent(QObject *parent) : QObject(parent)
{
    //timer.callOnTimeout(this, &QAgent::startCollectData);
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
//    if (!settings.value("Configuration").isNull())
//    {
//        auto confMap = settings.value("Configuration").toList();
//        for (const auto& now : confMap)
//        {
//            if (collectData.find(now.toString()) != end(collectData))
//                confBitMask &= collectData.at(now.toString());
//        }
//    }

    if (!settings.value("Configuration").isNull())
    {
        auto jsonArr = settings.value("Configuration").toJsonArray();
        for (const auto& jsonVal : jsonArr)
            if (!parseJsonConfig(jsonVal))
                qCritical() << "Unable to parse JSON configuration file!" << Qt::endl;

        for (auto& timer : timers)
            timer->start();
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
    qDebug()<<"Compression set: "<<newCompress;
    compression = newCompress;
}

bool QAgent::performPassiveCheck()
{
    // TODO
    return false;
}

bool QAgent::performActiveCheck()
{
    //timer.stop();
    // подготовка сообщения
    updateVirtualIds(*dataArray);
    QJsonArray jsonArray;
    for (auto const& it : *dataArray)
    {
        qDebug() << it.toJson();
        jsonArray.push_back(it.toJson());
    }
    QVariantMap payload
    {
        std::pair{"request", "agent data"},
        std::pair{"data", jsonArray},
        std::pair{"clock", static_cast<int>(std::time(nullptr))}
    };
    auto message = Utils::DataMessage{payload};

    // попытка подключиться к серверу
    openSocket();
    if (!query)
    {
        qCritical() << QTime::currentTime().toString(Qt::ISODateWithMs)
                    << "No connection to server" << Qt::flush;
        closeSocket();
        timer.start(refreshActiveChecks);
        return false;
    }
    performHandshake(query);
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
                << "Connected to server";


    // отправка сообщения
    auto response = query->makeQuery()
                          .toGet<Utils::Service>()
                          .toSend(message, compression)
                          .invoke();
    if (!response.has_value())
    {
        qWarning() << "Something went wrong! Trying send message again...";
        response = query->makeQuery()
                         .toGet<Utils::Service>()
                         .toSend(message, compression)
                         .invoke();
        if (!response.has_value())
        {
            qWarning() << "Failed...";
            closeSocket();
            timer.start(refreshActiveChecks);
            return false;
        }
    }
    if (response->response == QString("success"))
        qDebug() << "Success response";
    else qWarning() << "Something went wrong! Server response: " << response->response << Qt::flush;

    closeSocket();
    //timer.start(refreshActiveChecks);
    return true;
}


QAgent::~QAgent()
{
    for (auto& timer : timers)
        delete timer;
}

inline void QAgent::openSocket()
{
    if (!serverIP.isNull() and serverPort)
    {
        auto tcpSocket = std::make_shared<QTcpSocket>();
        tcpSocket->connectToHost(serverIP, serverPort);
        if (tcpSocket->waitForConnected(10000))
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
    /*qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Data collection started" << Qt::flush;
    auto dataStruct = OS_UTILS::OS_EVENTS::pullOSStatus(confBitMask);
    //auto localArray = toCollVec(dataStruct);

    for (auto const& it : localArray)
    {
        if (dataArray->size() >= bufferSize)
        {
            performActiveCheck();
            dataArray->clear();
        }
        dataArray->push_back(it);
    }

    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Data collection ended" << Qt::flush;*/
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
        {"macaddress", Utils::getMacAddress()},
        {"hostname", hostName}
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
    Q_UNUSED(status);
    /*collVec localVec;
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
    return localVec;*/
}

bool QAgent::addData(const QJsonValue& value, const QString& key)
{
    if (dataArray->size() == bufferSize)
    {
        performActiveCheck();
        dataArray->clear();
    }
    dataArray->push_back({
                             this->hostName,
                             key,
                             value,
                         });

    return true;
}

bool QAgent::parseJsonConfig(const QJsonValue& jsonVal)
{
    if (!jsonVal.isObject())
        return false;

    auto jsonObj = jsonVal.toObject();
    auto timer = new QTimer(this);
    QString str;
    std::chrono::milliseconds time;

    if (jsonObj.contains(Utils::AvailableNetworkDevices))
    {
        timer->callOnTimeout(this, &QAgent::getAvailableNetworkdevices);
        /*QTimer timer(this);
        timer.callOnTimeout(this, &QAgent::getAvailableNetworkdevices);

        auto str = jsonObj.value(Utils::AvailableNetworkDevices).toString();
        auto time = Utils::parseTime(jsonObj.value(Utils::AvailableNetworkDevices).toString());
        if (str.contains("SingleShot"))
            timer.setSingleShot(true);
        timer.setInterval(time);
        timers.push_back(timer);*/

        str = jsonObj.value(Utils::AvailableNetworkDevices).toString();
        time = Utils::parseTime(str);
        if (str.contains("SingleShot"))
            timer->setSingleShot(true);
        timer->setInterval(time);
        timers.push_back(timer);
    }
    else if (jsonObj.contains(Utils::CurrentMultiCoreUsage))
    {
        cpuMonitoring->initMultiCore();
        timer->callOnTimeout(this, &QAgent::currentMultiCoreUsage);

        str = jsonObj.value(Utils::CurrentMultiCoreUsage).toString();
        time = Utils::parseTime(str);
        if (str.contains("SingleShot"))
            timer->setSingleShot(true);
        timer->setInterval(time);
        timers.push_back(timer);
    }
    else if (jsonObj.contains(Utils::CurrentCoreUsage))
    {
        cpuMonitoring->initcpuUsage();
        timer->callOnTimeout(this, &QAgent::currentCoreUsage);

        str = jsonObj.value(Utils::CurrentCoreUsage).toString();
        time = Utils::parseTime(str);
        if (str.contains("SingleShot"))
            timer->setSingleShot(true);
        timer->setInterval(time);
        timers.push_back(timer);
    }
    else if (jsonObj.contains(Utils::NumOfCpus))
    {
        timer->callOnTimeout(this, &QAgent::numOfCPUs);

        str = jsonObj.value(Utils::NumOfCpus).toString();
        time = Utils::parseTime(str);
        if (str.contains("SingleShot"))
            timer->setSingleShot(true);
        timer->setInterval(time);
        timers.push_back(timer);
    }
    else if (jsonObj.contains(Utils::CpuName))
    {
        timer->callOnTimeout(this, &QAgent::CPUName);

        str = jsonObj.value(Utils::CpuName).toString();
        time = Utils::parseTime(str);
        if (str.contains("SingleShot"))
            timer->setSingleShot(true);
        timer->setInterval(time);
        timers.push_back(timer);
    }
    else if (jsonObj.contains(Utils::TotalMemInKb))
    {
        timer->callOnTimeout(this, &QAgent::totalMemoryInKB);

        str = jsonObj.value(Utils::TotalMemInKb).toString();
        time = Utils::parseTime(str);
        if (str.contains("SingleShot"))
            timer->setSingleShot(true);
        timer->setInterval(time);
        timers.push_back(timer);
    }
    else if (jsonObj.contains(Utils::CurrentMemUsageInKb))
    {
        timer->callOnTimeout(this, &QAgent::currentMemUsageInKB);

        str = jsonObj.value(Utils::CurrentMemUsageInKb).toString();
        time = Utils::parseTime(str);
        if (str.contains("SingleShot"))
            timer->setSingleShot(true);
        timer->setInterval(time);
        timers.push_back(timer);
    }
    else if (jsonObj.contains(Utils::CurrentMemUsageInPercent))
    {
        timer->callOnTimeout(this, &QAgent::currentMemUsageInPercent);

        str = jsonObj.value(Utils::CurrentMemUsageInPercent).toString();
        time = Utils::parseTime(str);
        if (str.contains("SingleShot"))
            timer->setSingleShot(true);
        timer->setInterval(time);
        timers.push_back(timer);
    }
    else
    {
        delete timer;
        return false;
    }
    return true;
}

// ********* CPU USAGE ********* //
bool QAgent::currentMultiCoreUsage()
{
    auto localVec = cpuMonitoring->getCurrentMultiCoreUsage();
    if (localVec.size() == 0)
        return false;
    QJsonArray value;
    std::copy(begin(localVec), end(localVec), std::back_inserter(value));
    qDebug() << value;
    return addData(value, Utils::CurrentMultiCoreUsage);
}

bool QAgent::currentCoreUsage()
{
    auto value = cpuMonitoring->getCurrentCpuUsage();
    return addData(value, Utils::CurrentCoreUsage);
}

bool QAgent::numOfCPUs()
{
    auto value = cpuMonitoring->getCores();
    return addData(QJsonValue::fromVariant(value), Utils::NumOfCpus);
}

bool QAgent::CPUName()
{
    auto value = cpuMonitoring->getCPUName();
    return addData(QString::fromStdString(value), Utils::CpuName);
}

// ********* MEMORY USAGE ********* //
bool QAgent::totalMemoryInKB()
{
    auto value = memoryMonitoring->getTotalMemoryInKB();
    return addData(QJsonValue::fromVariant(QVariant::fromValue(value)), Utils::TotalMemInKb);
}

bool QAgent::currentMemUsageInKB()
{
    auto value = memoryMonitoring->getCurrentMemUsageInKB();
    return addData(QJsonValue::fromVariant(QVariant::fromValue(value)), Utils::CurrentMemUsageInKb);
}

bool QAgent::currentMemUsageInPercent()
{
    auto value = memoryMonitoring->getCurrentMemUsageInPercent();
    return addData(value, Utils::CurrentMemUsageInPercent);
}


// ********* NETWORK USAGE ********* //
bool QAgent::getAvailableNetworkdevices()
{
    if (ethernetMonitoring.size() == 0)
        return false;
    QJsonArray value;
    for (const auto& it : ethernetMonitoring)
        value.push_back(QString::fromStdString(it->getDeviceName()));
    return addData(value, Utils::AvailableNetworkDevices);
}


