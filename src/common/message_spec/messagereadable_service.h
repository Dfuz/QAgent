#ifndef MESSAGREADABLE_SERVICE_H
#define MESSAGREADABLE_SERVICE_H

#include "messagereadable.h"
#include <optional>

namespace Utils
{

template<>
struct ReadableMessage<Utils::Service>
{
    static std::optional<ReadableMessage<Utils::Service>> parseJson(const QByteArray &data) noexcept
    {
        const auto obj = ReadableMessageHelper::verifyType<Utils::Service>(data);
        if (!obj)
            return std::nullopt;

        return ReadableMessage<Utils::Service>{};
    }    
};

}

#endif // MESSAGREADABLE_SERVICE_H