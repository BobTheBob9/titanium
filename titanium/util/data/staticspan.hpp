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



    // ////////////// //
    // IMPLEMENTATION //
    // ////////////// //

    template <typename T, u64 TSize>
    StaticSpan<T, TSize>::StaticSpan( const std::initializer_list<T> ptInitialValue )
    {
        memcpy( m_tData, ptInitialValue.begin(), std::min( ptInitialValue.size() * sizeof( T ), TSize * sizeof( T ) ) );
    }

    template <typename T, u64 TSize>
    u64 StaticSpan<T, TSize>::Elements() const
    {
        return TSize;
    }

    template <typename T, u64 TSize>
    u64 StaticSpan<T, TSize>::Size() const
    {
        return TSize * sizeof( T );
    }

    template <typename T, u64 TSize>
    Span<T> StaticSpan<T, TSize>::ToSpan() { return Span<T>( Elements(), m_tData ); }
    template <typename T, u64 TSize>
    const Span<T> StaticSpan<T, TSize>::ToConstSpan() { return Span<T>( Elements(), m_tData ); }

}
