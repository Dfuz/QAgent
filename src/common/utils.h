#ifndef UTILS_H
#define UTILS_H

#include <QSettings>
#include <QIODevice>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QDebug>

namespace Utils {

inline bool readJsonFile(QIODevice &device, QSettings::SettingsMap &map)
{
    QByteArray readedBytes = device.readAll();
    QJsonDocument resJson = QJsonDocument::fromJson(readedBytes);
    if (resJson.isNull()) {
        //TODO: handle error
        qWarning() << " got null";
        return false;
    }
    if (!resJson.isObject()) {
        qInfo() << resJson << " is not Object (wrong format)";
        return false;
    }
    map = resJson.object().toVariantMap();
    return true;
}

inline bool writeJsonFile(QIODevice &device, const QSettings::SettingsMap &map)
{
    auto resJsonObj = QJsonObject::fromVariantMap(map);
    QJsonDocument resJson(resJsonObj);
    auto written = device.write(resJson.toJson());
    if (written == -1) {
        return false;
    } else {
        qInfo() << "written " << written << " bytes";
        return true;
    }
}

const QSettings::Format JsonFormat = QSettings::registerFormat("json", readJsonFile, writeJsonFile);

inline std::chrono::milliseconds parseTime(const QString& input)
{
    std::chrono::milliseconds time{0};

    QRegularExpression ms{"[0-9]+ms"},
                       s{"[0-9]+s"},
                       min{"[0-9]+m"},
                       hour{"[0-9]+h"};

    time += std::chrono::hours{hour.match(input).captured().left(1).toInt()};
    time += std::chrono::minutes{min.match(input).captured().left(1).toInt()};
    time += std::chrono::seconds{s.match(input).captured().left(1).toInt()};
    time += std::chrono::milliseconds{ms.match(input).captured().left(2).toInt()};
    return time;
}

}

#endif // UTILS_H
