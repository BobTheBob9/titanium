#pragma once

#include <type_traits>

namespace util::maths
{
    template<typename T> requires std::is_arithmetic_v<T> || std::is_pointer_v<T>
    bool NumberWithinRange( const T tValue, const T tBegin, const T tEnd )
    {
        return tValue >= tBegin && tValue < tEnd;
    }
}
