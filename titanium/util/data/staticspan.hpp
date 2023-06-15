#pragma once

#include "titanium/util/numerics.hpp"

namespace utils::data
{
    /*
        Similar to a span, but all elements are stack-allocated, with the array itself having a fixed size in memory
    */
    template <typename T, u64 TSize>
    struct StaticSpan
    {
        T m_tData[ TSize ];
    };
}
