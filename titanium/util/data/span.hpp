#pragma once

#include <concepts>

#include "titanium/util/numerics.hpp"

namespace utils::data
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
    };  



    // ////////////// //
    // IMPLEMENTATION //
    // ////////////// //

    template <typename T, std::unsigned_integral TSize> 
    Span<T, TSize>::Span():
            m_nElements( 0 ),
            m_pData( nullptr )
        {}

    template <typename T, std::unsigned_integral TSize> 
    Span<T, TSize>::Span( const TSize nElements, T *const pData ) :
            m_nElements( nElements ),
            m_pData( pData )
        {}

    template <typename T, std::unsigned_integral TSize> 
    bool Span<T, TSize>::DefaultFind( const T tFirst, const T tSecond )
    {
        return tFirst == tSecond;
    }

    template <typename T, std::unsigned_integral TSize> 
    typename Span<T, TSize>::R_IndexOf_s Span<T, TSize>::IndexOf( const T tFindValue, const FnFindComparator fnComparator ) const
    {
        for ( TSize i = 0; i < m_nElements; i++ )
        {
            if ( fnComparator( m_pData[ i ], tFindValue ) )
            {
                return {
                    .bFound = true,
                    .nIndex = i
                };
            }
        }

        return {
            .bFound = false
        };
    }

    template <typename T, std::unsigned_integral TSize> 
    bool Span<T, TSize>::Contains( const T tFindValue, const FnFindComparator fnComparator ) const
    {
        return IndexOf( tFindValue, fnComparator ).bFound;
    }
}
