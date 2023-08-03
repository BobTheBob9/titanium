#pragma once

#include <initializer_list>

#include "titanium/util/numerics.hpp"
#include "titanium/util/data/span.hpp"

namespace util::data
{
    /*
        Similar to a span, but all elements are stack-allocated, with the array itself having a fixed size in memory
    */
    template <typename T, u64 TSize>
    struct StaticSpan
    {
        T m_tData[ TSize ];

        StaticSpan() = default;
        StaticSpan( const std::initializer_list<T> ptInitialValue );
        u64 Elements() const;
        u64 Size() const;

        Span<T> ToSpan();
        const Span<T> ToConstSpan();
    };
}

#include "staticspan_impl.inl"
