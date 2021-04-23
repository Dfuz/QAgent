#ifndef MESSAGREADABLE_SERVICE_H
#define MESSAGREADABLE_SERVICE_H

#include "messagereadable.h"
#include <optional>

namespace Utils
{
template<>
struct ReadableMessage<Utils::Service>
{
   QString response;

    static std::optional<ReadableMessage<Utils::Service>> parseJson(const QByteArray &data) noexcept
    {
        const auto obj = ReadableMessageHelper::verifyType<Utils::Service>(data);
        if (!obj)
            return std::nullopt;

        ReadableMessage<Utils::Service> retval{};

        if(!obj->contains("response"))
            return std::nullopt;
        retval.response = obj->value("response").toString();

        return retval;
    }    
};

}

#endif // MESSAGREADABLE_SERVICE_H
