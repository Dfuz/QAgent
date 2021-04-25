#ifndef DATA_H
#define DATA_H

#include "utils.h"
#include <QJsonObject>
#include <QString>
#include <string>
#include <ctime>
#include <map>

namespace Utils
{

struct CollectableData
{
    QString hostName; // видимое имя узла сети, которому принадлежит элемент данных
    QString keyData; // тип данных
    QJsonValue value;
    quint16 virtualId = 0; // используется, чтобы отбрасывать дубликаты значений, которые могут быть отправлены в средах с плохой связью
    std::time_t clock = std::time(nullptr);

    const QJsonObject toJson() const
    {
        return
        {
            {"hostname", hostName},
            {"key", keyData},
            {"value", value.toString()},
            {"id", virtualId},
            {"clock", static_cast<int>(clock)}
        };
    }
};

}
#endif // DATA_H
