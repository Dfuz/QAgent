#ifndef MESSAGREADABLE_NOMESSAGE_H
#define MESSAGREADABLE_NOMESSAGE_H

#include "messagereadable.h"
#include <optional>

namespace Utils
{

template<>
struct ReadableMessage<Utils::NoMessage>
{
    static std::optional<ReadableMessage<Utils::NoMessage>> parseJson(const QByteArray &data) noexcept
    {
        const auto obj = ReadableMessageHelper::verifyType<Utils::NoMessage>(data);
        if (!obj)
            return std::nullopt;
        return ReadableMessage<Utils::NoMessage>{};
    }    
};

}

#endif //MESSAGREADABLE_NOMESSAGE_H