namespace util::data
{
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
