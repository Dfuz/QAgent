#ifndef DATA_H
#define DATA_H

#include "utils.h"
#include <string>
#include <chrono>
#include <map>

namespace Utils
{

struct CollectableData
{
    std::string hostName; // видимое имя узла сети, которому принадлежит элемент данных
    Utils::DataTypes dataType; // тип данных
    quint16 virtualId; // используется, чтобы отбрасывать дубликаты значений, которые могут быть отправлены в средах с плохой связью
    std::chrono::time_point<std::chrono::system_clock> clock = std::chrono::system_clock::now();
    std::string value;
};

}
#endif // DATA_H
