#ifndef QAGENT_H
#define QAGENT_H

#include "common/querybuilder.h"
#include "common/utils.h"
#include "common/data.h"

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDebug>
#include <QTcpServer>
#include <QTimer>
#include <memory>
#include <chrono>
#include <map>

using namespace std::chrono;

class QAgent : public QObject
{
    Q_OBJECT
private:
    // Поля
    QTimer timer;
    quint16 serverPort{0};
    quint16 listenPort{0}; // агент будет слушать этот порт для подключений с сервера; диапазон 1024-32767
    QHostAddress serverIP{QHostAddress::Null};
    QHostAddress listenIP{QHostAddress::LocalHost};
    QString hostName; // уникальное, регистрозависимое имя хоста
    std::unique_ptr<Utils::QueryBuilder> query;
    QTcpServer localServer;
    inline static int compression;
    quint16 bufferSize = 100; // максимальное количество значений в буфере памяти
    std::chrono::seconds refreshActiveChecks{60s};
    std::map<Utils::DataTypes, bool> collectData; // список собираемых значений
    std::vector<Utils::CollectableData> dataArray; // список собранных значений

    // Методы
    void initSocket();
    void startCollectData();
    void performHandshake(std::shared_ptr<Utils::QueryBuilder> _query);
public:
    explicit QAgent(QObject *parent = nullptr);
    void readConfig(QString settings_path = "conf.json");
    bool startListen();
    void init();
    static int getCompression(void);
    static void setCompression(int newCompress);
    friend class qagent_tests;

private slots:
    bool performPassiveCheck();
signals:

};

#endif // QAGENT_H
