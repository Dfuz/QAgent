#ifndef QAGENT_H
#define QAGENT_H

#include "common/querybuilder.h"
#include "common/utils.h"
#include "common/data.h"
#include <os_utils.h>

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QCryptographicHash>
#include <QJsonArray>
#include <Qt>
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

// список собираемых значений
static const map<QString, Utils::DataTypes> collectData =
{
    {"FileSystem", Utils::DataTypes::FileSystem},
    {"Proccess", Utils::DataTypes::Process},
    {"Memory", Utils::DataTypes::Memory}
};

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
    std::chrono::milliseconds refreshActiveChecks{60s};             // значение таймера для активных проверок
    int confBitMask = 0b111; // маска конфигурации (какие параметры считывать)
    inline static int compression;
    QString macAddress{Utils::getMacAddress()};

    // Методы
    void openSocket();          // инициализация сокета для активных проверок
    void closeSocket();         // закрытие сокеты
    bool startListen();         // инициализация и запуск локального сервера
    void startCollectData();    // начать сбор данных (активные проверки)
    void updateVirtualIds(collVec& vec);    // обновляет виртуальные id в массиве данных
    void performHandshake(std::unique_ptr<Utils::QueryBuilder>& _query);
    collVec toCollVec(const OS_UTILS::OS_STATUS& status) const;

public:
    explicit QAgent(QObject *parent = nullptr);
    void readConfig(QString settings_path = "conf.json");
    void startAgent();
    void init();
    static int getCompression(void);
    static void setCompression(int newCompress);
    friend class qagent_tests;

private slots:
    bool performPassiveCheck();
    bool performActiveCheck();
};

#endif // QAGENT_H
