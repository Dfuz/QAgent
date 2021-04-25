#ifndef QAGENT_H
#define QAGENT_H

#include "common/querybuilder.h"
#include "common/utils.h"
#include "common/data.h"
#include <linux_cpuload.hpp>
#include <linux_memoryload.hpp>
#include <linux_networkload.hpp>
#include <linux_systemutil.hpp>
#include <util/record_value.hpp>

#include <Qt>
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QVector>
#include <QTextStream>
#include <QDebug>
#include <QTcpServer>
#include <QTimer>
#include <memory>
#include <chrono>
#include <map>

using namespace std::chrono;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::string;
using std::map;
using collVec = vector<Utils::CollectableData>;

static auto memoryMonitoring    = std::make_unique<memoryLoad>();
static auto cpuMonitoring       = std::make_unique<cpuLoad>();
static auto ethernetMonitoring  = networkLoad::createLinuxEthernetScanList();

class QAgent : public QObject
{
    Q_OBJECT
private:
    // Поля
    QTimer timer;
    QString hostName;           // уникальное, регистрозависимое имя хоста
    quint16 serverPort{0};      // порт сервера
    quint16 listenPort{10050};  // агент будет слушать этот порт для подключений с сервера; диапазон 1024-32767
    quint16 bufferSize = 100;   // максимальное количество значений в буфере памяти
    QHostAddress serverIP{QHostAddress::Null};      // адрес сервера для активных проверок
    QHostAddress listenIP{QHostAddress::Null};      // адрес, который должен слушать агент
    unique_ptr<Utils::QueryBuilder> query;
    unique_ptr<QTcpServer> localServer;                             // локальный сервер для пассивных проверок
    unique_ptr<collVec> dataArray = std::make_unique<collVec>();    // список собранных значений
    inline static int compression;
    QString macAddress{Utils::getMacAddress()};
    QVector<QTimer*> timers;

    // Методы
    void openSocket();                      // инициализация сокета для активных проверок
    void closeSocket();                     // закрытие сокеты
    bool startListen();                     // инициализация и запуск локального сервера
    void updateVirtualIds(collVec& vec);    // обновляет виртуальные id в массиве данных
    bool addActiveCheck(const QJsonObject&);
    void performHandshake(std::unique_ptr<Utils::QueryBuilder>& _query);
    bool addData(const QJsonValue& value, const QString& key);
    bool parseJsonConfig(const QJsonValue&);
    void stopAllTimers();
    void runAllTimers();
    void setupTimer(const QString& string,
                    QTimer* timer,
                    const QJsonObject& obj);

public:
    explicit QAgent(QObject *parent = nullptr);
    ~QAgent();
    void readConfig(QString settings_path = "conf.json");
    void startAgent();
    void init();
    static int getCompression(void);
    static void setCompression(int newCompress);
    friend class qagent_tests;

private slots:
    bool performPassiveCheck();
    bool performActiveCheck();

public slots:
    // CPU_LOAD
    bool currentMultiCoreUsage();
    bool currentCoreUsage();
    bool numOfCPUs();
    bool CPUName();

    // MEMORY
    bool totalMemoryInKB();
    bool currentMemUsageInKB();
    bool currentMemUsageInPercent();

    // NETWORK
    bool getAvailableNetworkdevices();
};

#endif // QAGENT_H
