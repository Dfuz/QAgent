#ifndef MESSAGREADABLE_HANDSHAKE_H
#define MESSAGREADABLE_HANDSHAKE_H

#include "messagereadable.h"
#include <optional>
#include <qhostaddress.h>

namespace Utils
{

template<>
struct ReadableMessage<Utils::Handshake>
{
    QString who;
    QString hostname;
    int compressionLevel;

    static std::optional<ReadableMessage<Utils::Handshake>> parseJson(const QByteArray &data) noexcept
    {
        const auto obj = ReadableMessageHelper::verifyType<Utils::Handshake>(data);
        if (!obj)
            return std::nullopt;
        
        ReadableMessage<Utils::Handshake> retval{};
        
        if (!obj->contains("who")) return std::nullopt;
        retval.who = obj->value("who").toString();
        retval.hostname = obj->value("hostname").toString();
        retval.compressionLevel = obj->value("compression").toInt(0);

        return retval;
    }    
};

}

#endif //MESSAGREADABLE_HANDSHAKE_H
