#ifndef QAGENT_H
#define QAGENT_H

#include "common/querybuilder.h"

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <memory>

class QAgent : public QObject
{
    Q_OBJECT
private:
    QTimer timer;
    quint16 serverPort{0};
    QHostAddress serverAddress{QHostAddress::Null};
    std::unique_ptr<Utils::QueryBuilder> query;
    inline static int compression;
    void initSocket();
    void startSend();
    void performHandshake(std::shared_ptr<Utils::QueryBuilder> _query);
public:
    explicit QAgent(QObject *parent = nullptr);
    void init();
    static int getCompression(void);
    static void setCompression(int newCompress);
    friend class qagent_tests;

signals:

};

#endif // QAGENT_H
