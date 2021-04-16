#ifndef MESSAGREADABLE_DATA_H
#define MESSAGREADABLE_DATA_H

#include "messagereadable.h"
#include <optional>

namespace Utils
{

template<>
struct ReadableMessage<Utils::Data>
{
    static std::optional<ReadableMessage<Utils::Data>> parseJson(const QByteArray &data) noexcept
    {
        const auto obj = ReadableMessageHelper::verifyType<Utils::Data>(data);
        if (!obj)
            return std::nullopt;
        
        ReadableMessage<Utils::Data> retval{};

        return retval;
    }    
};

}

#endif // MESSAGREADABLE_DATA_H