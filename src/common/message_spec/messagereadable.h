#ifndef MESSAGREADABLE_H
#define MESSAGREADABLE_H

#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <memory>
#include <optional>

#include "messagetypes.h"

namespace Utils
{

struct ReadableMessageHelper
{
    template<MessageType type>
    static std::optional<QJsonObject> verifyType(const QByteArray &data) noexcept
    {
        auto msg = QJsonDocument::fromJson(data);

        qDebug() << "parsing Json";

        if (!msg.isObject())
            return std::nullopt;

        auto msgObject = msg.object();
        if (!msgObject.value("type").isDouble())
            return std::nullopt;

        auto gotType = static_cast<MessageType>(msgObject.value("type").toInt());
        if (gotType != type)
            return std::nullopt;

        msgObject.take("type");
        return msgObject;
    }
};

template<MessageType type>
struct ReadableMessage
{
    constexpr static std::optional<ReadableMessage<type>> parseJson(const QByteArray &data) noexcept = delete;
};

}

#endif //MESSAGREADABLE_H