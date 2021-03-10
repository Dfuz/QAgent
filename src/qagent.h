#ifndef QAGENT_H
#define QAGENT_H

#include <QObject>

class QAgent : public QObject
{
    Q_OBJECT
public:
    explicit QAgent(QObject *parent = nullptr);

signals:

};

#endif // QAGENT_H
