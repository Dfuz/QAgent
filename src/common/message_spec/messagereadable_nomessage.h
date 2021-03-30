#ifndef MESSAGREADABLE_NOMESSAGE_H
#define MESSAGREADABLE_NOMESSAGE_H

#include "messagereadable.h"
#include <optional>

namespace Utils
{

template<>
struct ReadableMessage<NoMessage>
{
    static std::optional<ReadableMessage<NoMessage>> parseJson(const QByteArray &data) noexcept
    {
        const auto obj = ReadableMessageHelper::verifyType<NoMessage>(data);
        if (!obj)
            return std::nullopt;
        ReadableMessage<NoMessage> retval{};
        return retval;
    }    
};

}

#endif //MESSAGREADABLE_NOMESSAGE_H