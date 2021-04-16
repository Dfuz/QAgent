#ifndef MESSAGREADABLE_TEST_H
#define MESSAGREADABLE_TEST_H

#include "messagereadable.h"
#include <optional>

namespace Utils
{

template<>
struct ReadableMessage<Utils::Test>
{
    static std::optional<ReadableMessage<Utils::Test>> parseJson(const QByteArray &data) noexcept
    {
        const auto obj = ReadableMessageHelper::verifyType<Utils::Test>(data);
        if (!obj)
            return std::nullopt;

        return ReadableMessage<Utils::Test>{};
    }    
};

}

#endif // MESSAGREADABLE_TEST_H