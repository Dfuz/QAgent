#ifndef MESSAGEBUILDER_H
#define MESSAGEBUILDER_H

#include <QVariant>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <memory>

namespace Utils
{

enum MessageType
{
    Service,
    Data,
    Test,
    NoMessage
};

template<MessageType type>
struct Message
{
    Message() {}
    Message(const QString &str)
    {
        QJsonParseError err;
        auto object = QJsonDocument::fromJson(str.toUtf8(), &err).object();
        payload = object.toVariantMap();
        qDebug() << "parsed " << payload << " with obj " << object;
        qDebug() << "error " << err.errorString();
    }
    Message(const QVariantMap& map): payload(map) {}
    Message(const QJsonObject& obj): payload(obj.toVariantMap()) {}
    QVariantMap payload;

    QByteArray toJson() const
    {
        QVariantMap res{payload};
        res.insert("type", static_cast<int>(type));
        return QJsonDocument{QJsonObject::fromVariantMap(res)}.toJson(QJsonDocument::Compact);
    }

    static std::optional<Message<type>> parseJson(const QByteArray &data) noexcept
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

        qDebug() << "before return "<<msgObject;
        return Message{msgObject.toVariantMap()};
    }
};

using TestMessage = Message<MessageType::Test>;
using DataMessage = Message<MessageType::Data>;
using ServiceMessage = Message<MessageType::Service>;

}

template<Utils::MessageType type>
QDebug operator<<(QDebug dbg, const Utils::Message<type>& message)
{
    dbg.nospace() << "Type: " << type;
    dbg.nospace() << "Payload: " << message.payload;
    return dbg.maybeSpace();
}

#endif // MESSAGEBUILDER_H
