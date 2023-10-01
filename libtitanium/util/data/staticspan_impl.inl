#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <libtitanium/util/maths.hpp>

namespace util::data
{
    template <typename T, u64 TSize>
    StaticSpan<T, TSize>::StaticSpan( const std::initializer_list<T> ptInitialValue )
    {
        memcpy( m_tData, ptInitialValue.begin(), util::maths::Min( ptInitialValue.size() * sizeof( T ), TSize * sizeof( T ) ) );
    }

    template <typename T, u64 TSize>
    constexpr u64 StaticSpan<T, TSize>::Elements() const
    {
        return TSize;
    }

    template <typename T, u64 TSize>
    constexpr u64 StaticSpan<T, TSize>::Size() const
    {
        return TSize * sizeof( T );
    }

    template <typename T, u64 TSize>
    Span<T> StaticSpan<T, TSize>::ToSpan() { return Span<T>( Elements(), m_tData ); }
    template <typename T, u64 TSize>
    const Span<T> StaticSpan<T, TSize>::ToConstSpan() { return Span<T>( Elements(), m_tData ); }

    template <u64 TSize> StaticSpan<char, TSize> StaticSpan_FromString( const char *const pszFmt, ... )
    {
        StaticSpan<char, TSize> r_szString {};

        // todo: should this assert if we hit the buffer? (i.e. when output truncated)
        // i don't like manually doing this, having some kind of macro would be nice
        va_list vargs;
        va_start( vargs, pszFmt );
        (void)vsnprintf( r_szString.m_tData, TSize, pszFmt, vargs );
        va_end( vargs );

        return r_szString;
    }
}
