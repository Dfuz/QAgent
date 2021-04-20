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
    Utils::DataTypes dataType; // тип данных
    quint16 virtualId; // используется, чтобы отбрасывать дубликаты значений, которые могут быть отправлены в средах с плохой связью
    std::time_t clock = std::time(nullptr);
    QString value;

    const QJsonObject toJson() const
    {
        return
        {
            {"hostname", hostName},
            {"id", virtualId},
            {"data", dataType},
            {"clock", QString::number(clock)},
            {"value", value}
        };
    }
};

}
#endif // DATA_H
