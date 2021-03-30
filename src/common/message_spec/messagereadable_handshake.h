#ifndef MESSAGREADABLE_HANDSHAKE_H
#define MESSAGREADABLE_HANDSHAKE_H

#include "messagereadable.h"
#include <optional>

namespace Utils
{

template<>
struct ReadableMessage<Handshake>
{
    int compressionLevel;

    static std::optional<ReadableMessage<Handshake>> parseJson(const QByteArray &data) noexcept
    {
        const auto obj = ReadableMessageHelper::verifyType<Handshake>(data);
        if (!obj)
            return std::nullopt;
        
        ReadableMessage<Handshake> retval{};
        retval.compressionLevel = obj->value("compression").toInt(0);

        return retval;
    }    
};

}

#endif //MESSAGREADABLE_HANDSHAKE_H