#ifndef MESSAGESENDABLE_H
#define MESSAGESENDABLE_H

#include <QVariant>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

#include "messagetypes.h"

namespace Utils
{

template<MessageType type>
struct SendableMessage
{
    SendableMessage() {}
    SendableMessage(const QString &str)
    {
        QJsonParseError err;
        auto object = QJsonDocument::fromJson(str.toUtf8(), &err).object();
        payload = object.toVariantMap();
        qDebug() << "parsed " << payload << " with obj " << object;
        qDebug() << "error " << err.errorString();
    }
    SendableMessage(const QVariantMap& map): payload(map) {}
    SendableMessage(const QJsonObject& obj): payload(obj.toVariantMap()) {}
    QVariantMap payload;

    QByteArray toJson() const
    {
        QVariantMap res{payload};
        res.insert("type", static_cast<int>(type));
        return QJsonDocument{QJsonObject::fromVariantMap(res)}.toJson(QJsonDocument::Compact);
    }
};

using HandshakeMessage = SendableMessage<Handshake>;
using ServiceMessage = SendableMessage<Service>;
using DataMessage = SendableMessage<Data>;
using TestMessage = SendableMessage<Test>;
using NoMsgMessage = SendableMessage<NoMessage>;

}

template<Utils::MessageType type>
QDebug operator<<(QDebug dbg, const Utils::SendableMessage<type>& message)
{
    dbg.nospace() << "Type: " << type;
    dbg.nospace() << "Payload: " << message.payload;
    return dbg.maybeSpace();
}

#endif //MESSAGESENDABLE_H