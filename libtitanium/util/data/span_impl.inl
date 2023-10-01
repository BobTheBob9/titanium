namespace util::data
{
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

    template <typename T, std::unsigned_integral TSize> 
    Span<T, TSize> Span<T, TSize>::Offset( const TSize nOffset )
    {
        return Span<T, TSize>( m_nElements - nOffset, m_pData + nOffset );
    }

    template <typename T, std::unsigned_integral TSize>
    Span<T, TSize> Span<T, TSize>::Slice( const TSize nFirstIndex, const TSize nLength )
    {
        return Span<T, TSize>( nLength, m_pData + nFirstIndex );
    }
}
