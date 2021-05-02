#include "qagent.h"
#include "common/utils.h"

QAgent::QAgent(QObject *parent) : QObject(parent)
{}

QAgent::~QAgent()
{
    for (auto& timer : timers)
        delete timer;
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

    if (!settings.value("Configuration").isNull())
    {
        auto jsonArr = settings.value("Configuration").toJsonArray();
        for (const auto& jsonVal : jsonArr)
            if (!parseJsonConfig(jsonVal))
                qCritical() << "Unable to parse JSON configuration file!" << Qt::endl;

        for (auto& timer : timers)
            timer->start();
    }
}

void QAgent::setHostName(const QString &newName)
{
    qDebug() << "New hostname: " << newName;
    hostName = newName;
}

void QAgent::startAgent()
{
    runAllTimers();
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Active checks started!";

    if (listenIP != QHostAddress::Null)
    {
        startListen();
        qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
                 << "Passive checks started!";
    }
}

void QAgent::setupTimer(const QString& string, QTimer* timer, const QJsonObject& obj)
{
    const auto str = obj.value(string).toString();
    const auto time = Utils::parseTime(str);
    if (str.contains("SingleShot"))
        timer->setSingleShot(true);
    timer->setInterval(time);
    timers.push_back(timer);
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
            return false;
        }
    }
    if (response->response == QString("success"))
        qDebug() << "Success response";
    else qWarning() << "Something went wrong! Server response: " << response->response << Qt::flush;

    closeSocket();
    return true;
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
    auto msg = _query->makeQuery()
           .toSend(message)
           .toGet<Utils::Handshake>() 
           .invoke();
    if (msg)
    {
        QAgent::setCompression(msg->compressionLevel);
    }
}

inline bool QAgent::addData(const QJsonValue& value, const QString& key)
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

    if (jsonObj.contains(Utils::AvailableNetworkDevices))
    {
        timer->callOnTimeout(this, &QAgent::getAvailableNetworkdevices);
        setupTimer(Utils::AvailableNetworkDevices, timer, jsonObj);
    }
    else if (jsonObj.contains(Utils::CurrentMultiCoreUsage))
    {
        cpuMonitoring->initMultiCore();
        timer->callOnTimeout(this, &QAgent::currentMultiCoreUsage);      
        setupTimer(Utils::CurrentMultiCoreUsage, timer, jsonObj);
    }
    else if (jsonObj.contains(Utils::CurrentCoreUsage))
    {
        cpuMonitoring->initcpuUsage();
        timer->callOnTimeout(this, &QAgent::currentCoreUsage);
        setupTimer(Utils::CurrentCoreUsage, timer, jsonObj);
    }
    else if (jsonObj.contains(Utils::NumOfCpus))
    {
        timer->callOnTimeout(this, &QAgent::numOfCPUs);
        setupTimer(Utils::NumOfCpus, timer, jsonObj);
    }
    else if (jsonObj.contains(Utils::CpuName))
    {
        timer->callOnTimeout(this, &QAgent::CPUName);
        setupTimer(Utils::CpuName, timer, jsonObj);
    }
    else if (jsonObj.contains(Utils::TotalMemInKb))
    {
        timer->callOnTimeout(this, &QAgent::totalMemoryInKB);
        setupTimer(Utils::TotalMemInKb, timer, jsonObj);
    }
    else if (jsonObj.contains(Utils::CurrentMemUsageInKb))
    {
        timer->callOnTimeout(this, &QAgent::currentMemUsageInKB);
        setupTimer(Utils::CurrentMemUsageInKb, timer, jsonObj);
    }
    else if (jsonObj.contains(Utils::CurrentMemUsageInPercent))
    {
        timer->callOnTimeout(this, &QAgent::currentMemUsageInPercent);
        setupTimer(Utils::CurrentMemUsageInPercent, timer, jsonObj);
    }
    else
    {
        delete timer;
        return false;
    }
    return true;
}

void QAgent::stopAllTimers()
{
    for (auto& timer : timers)
        timer->stop();
}

void QAgent::runAllTimers()
{
    for (auto& timer : timers)
        timer->start();
}

// ********* CPU USAGE ********* //
bool QAgent::currentMultiCoreUsage()
{
    auto localVec = cpuMonitoring->getCurrentMultiCoreUsage();
    if (localVec.size() == 0)
        return false;
    QJsonArray value;
    std::copy(begin(localVec), end(localVec), std::back_inserter(value));
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Current multicore usage";
    return addData(value, Utils::CurrentMultiCoreUsage);
}

bool QAgent::currentCoreUsage()
{
    auto value = cpuMonitoring->getCurrentCpuUsage();
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Current core usage";
    return addData(value, Utils::CurrentCoreUsage);
}

bool QAgent::numOfCPUs()
{
    auto value = cpuMonitoring->getCores();
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Number of processor cores";
    return addData(static_cast<quint16>(value), Utils::NumOfCpus);
}

bool QAgent::CPUName()
{
    auto value = cpuMonitoring->getCPUName();
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "CPU name";
    return addData(QString::fromStdString(value), Utils::CpuName);
}

// ********* MEMORY USAGE ********* //
bool QAgent::totalMemoryInKB()
{
    auto value = memoryMonitoring->getTotalMemoryInKB();
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Total memory in KB";
    return addData(static_cast<qint64>(value), Utils::TotalMemInKb);
}

bool QAgent::currentMemUsageInKB()
{
    auto value = memoryMonitoring->getCurrentMemUsageInKB();
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Current memory usage in KB";
    return addData(static_cast<qint64>(value), Utils::CurrentMemUsageInKb);
}

bool QAgent::currentMemUsageInPercent()
{
    auto value = memoryMonitoring->getCurrentMemUsageInPercent();
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Current memory usage in percent";
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
    qDebug() << QTime::currentTime().toString(Qt::ISODateWithMs)
             << "Available network devices";
    return addData(value, Utils::AvailableNetworkDevices);
}
