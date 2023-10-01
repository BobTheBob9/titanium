#pragma once

#include <initializer_list>

#include <libtitanium/util/numerics.hpp>
#include <libtitanium/util/data/span.hpp>

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
        constexpr u64 Elements() const;
        constexpr u64 Size() const;

        Span<T> ToSpan();
        const Span<T> ToConstSpan();
    };

    template <u64 TSize> StaticSpan<char, TSize> StaticSpan_FromString( const char *const pszFmt, ... );
}

#include "staticspan_impl.inl"
