#pragma once

#include <concepts>

#include <libtitanium/util/numerics.hpp>

namespace util::data
{
    /*
    
    Wrapper around c-style arrays with an included length field
    
    */
    template <typename T, std::unsigned_integral TSize = u32>
    struct Span
    {
        TSize m_nElements;
        T* m_pData;

        Span();
        Span( const TSize nElements, T *const pData );

        using FnFindComparator = bool(*)( const T tFirst, const T tSecond );
        static bool DefaultFind( const T tFirst, const T tSecond );

        struct R_IndexOf_s
        {
            bool bFound;
            TSize nIndex;
        };
        
        R_IndexOf_s IndexOf( const T tFindValue, const FnFindComparator fnComparator = DefaultFind ) const;
        bool Contains( const T tFindValue, const FnFindComparator fnComparator = DefaultFind ) const;

        Span<T, TSize> Offset( const TSize nOffset );
        Span<T, TSize> Slice( const TSize nFirstIndex, const TSize nLength );
    };  
}

#include "span_impl.inl"
