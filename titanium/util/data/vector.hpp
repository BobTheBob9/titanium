#pragma once

#include <concepts>
#include <cstring>

#include "titanium/memory/mem_core.hpp"
#include "titanium/util/numerics.hpp"
#include "titanium/util/data/span.hpp"
#include "titanium/util/assert.hpp"

namespace utils::data
{
    template <typename T, std::unsigned_integral TSize = u32>
    class Vector
    {
    private:
        T* m_pElements = nullptr;
        TSize m_nLastElement = 0;
        TSize m_nAllocatedElements = 0;

        TSize m_nAbsoluteMaxElements = maxof( TSize );

    public:
        ~Vector();

        void AppendWithAlloc( const T tElement );
        void AppendNoAlloc( const T tElement );
        void AppendMultipleWithAlloc( const Span<T, u32> tElements );
        void AppendMultipleNoAlloc( const Span<T, u32> tElements );

        void Remove( const T tElement );
        void RemoveAt( const TSize nIndex );
        void RemoveMultipleAt( const TSize nIndex, const TSize nElements );

        void SetAllocated( const TSize nElements );
        void SetAllocatedLoose( const TSize nElements );
        void SetAbsoluteMax( const TSize nMaxElements );

        T* GetAt( const TSize nIdx );
        const T* GetForReadAt( const TSize nIdx ) const;

        using FnFindComparator = bool(*)( const T tFirst, const T tSecond );
        static bool DefaultFind( const T tFirst, const T tSecond );

        struct R_IndexOf_s
        {
            bool bFound;
            TSize nIndex;
        };

        R_IndexOf_s IndexOf( const T tFindValue, const FnFindComparator fnComparator = DefaultFind ) const;
        bool Contains( const T tFindValue, const FnFindComparator fnComparator = DefaultFind ) const;

        inline T* Data();
        inline const T* DataForReadOnly() const;
        inline TSize Length() const;
        inline TSize ElementsAllocated() const;
    };



    // ////////////// //
    // IMPLEMENTATION //
    // ////////////// //

    // TODO: need to handle removal properly

    template<typename T, std::unsigned_integral TSize>
    Vector<T, TSize>::~Vector()
    {
        //memory::free( m_pElements );
    }

    template<typename T, std::unsigned_integral TSize>
    void Vector<T, TSize>::AppendWithAlloc( const T tElement )
    {
        if ( m_nLastElement + 1 > m_nAllocatedElements )
        {
            SetAllocatedLoose( m_nAllocatedElements + 1 );
        }

        AppendNoAlloc( tElement );
    }

    template<typename T, std::unsigned_integral TSize>
    void Vector<T, TSize>::AppendNoAlloc( const T tElement )
    {
        assert::Release( m_nLastElement + 1 <= m_nAllocatedElements );
        m_pElements[ m_nLastElement++ ] = tElement;
    }

    template<typename T, std::unsigned_integral TSize>
    void Vector<T, TSize>::AppendMultipleWithAlloc( const Span<T, u32> tElements )
    {
        if ( m_nLastElement + tElements.m_nElements < m_nAllocatedElements )
        {
            SetAllocatedLoose( m_nAllocatedElements + tElements.m_nElements );
        }

        AppendMultipleNoAlloc( tElements );
    }

    template<typename T, std::unsigned_integral TSize>
    void Vector<T, TSize>::AppendMultipleNoAlloc( const Span<T, u32> tElements )
    {
        assert::Release( m_nLastElement + tElements.m_nElements <= m_nAllocatedElements );
        // TODO: map to inline func in memory::
        ::memcpy( m_pElements + m_nLastElement, tElements.m_pData, tElements.m_nElements * sizeof(T) );
        m_nLastElement += tElements.m_nElements;
    }

    template<typename T, std::unsigned_integral TSize>
    void Vector<T, TSize>::Remove( const T tElement )
    {
        for ( TSize i = 0; i < Length(); i++ )
        {
            if ( m_pElements[ i ] == tElement )
            {
                RemoveAt( i );
                return;
            }
        }
    }

    template<typename T, std::unsigned_integral TSize>
    void Vector<T, TSize>::RemoveAt( const TSize nIndex )
    {
        assert::Release( nIndex <= m_nLastElement, "Tried to remove element %i which is beyond bounds of vector! (%i elements)", nIndex, m_nLastElement );
        // TODO: map to inline func in memory::
        memmove( m_pElements + nIndex, m_pElements + nIndex + 1, m_nLastElement - nIndex );
        m_nLastElement--;
        
        SetAllocatedLoose( m_nLastElement );
    }

    template<typename T, std::unsigned_integral TSize>
    void Vector<T, TSize>::RemoveMultipleAt( const TSize nIndex, const TSize nElements )
    {
        // TODO: map to inline func in memory::
        memmove( m_pElements + nIndex, m_pElements + nIndex + nElements , m_nLastElement - ( nIndex + nElements ) );
        m_nLastElement -= nElements;

        SetAllocatedLoose( m_nLastElement );
    }

    template<typename T, std::unsigned_integral TSize>
    void Vector<T, TSize>::SetAllocated( const TSize nElements )
    {
        if ( nElements == m_nAllocatedElements )
            return;

        if ( !nElements )
        {
            memory::free( m_pElements );
            return;
        }

        assert::Release( nElements <= m_nAbsoluteMaxElements, "Tried to allocate %i elements when m_nAbsoluteMaxElements is %i", nElements, m_nAbsoluteMaxElements );
        assert::Release( nElements >= m_nLastElement, "Tried to allocate %i elements when m_nLastElement is %i", nElements, m_nLastElement );
    
        if ( !m_pElements )
        {
            m_pElements = memory::alloc_nT<T>( nElements );
        }
        else
        {
            m_pElements = memory::realloc_nT<T>( m_pElements, nElements );
        }

        m_nAllocatedElements = nElements;
    }

    template<typename T, std::unsigned_integral TSize>
    void Vector<T, TSize>::SetAllocatedLoose( const TSize nElements )
    {
        // SetReserved( closest power of 2 > nElements )
        SetAllocated( pow( 2, ceil( log( nElements ) / log( 2 ) ) ) );  
    }

    template<typename T, std::unsigned_integral TSize>
    T* Vector<T, TSize>::GetAt( const TSize nIdx )
    {
        assert::Release( nIdx < m_nLastElement, "Tried to access index %i when m_nLastElement is %i", nIdx, m_nLastElement );
        return m_pElements[ nIdx ];
    }

    template<typename T, std::unsigned_integral TSize>
    const T* Vector<T, TSize>::GetForReadAt( const TSize nIdx ) const
    {
        return const_cast<const T*>( GetAt( nIdx ) );
    }

    template <typename T, std::unsigned_integral TSize> 
    bool Vector<T, TSize>::DefaultFind( const T tFirst, const T tSecond )
    {
        return tFirst == tSecond;
    }

    template<typename T, std::unsigned_integral TSize>
    typename Vector<T, TSize>::R_IndexOf_s Vector<T, TSize>::IndexOf( const T tFindValue, const FnFindComparator fnComparator ) const
    {
        for ( TSize i = 0; i < Length(); i++ )
        {
            if ( fnComparator( m_pElements[ i ], tFindValue ) )
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

    template<typename T, std::unsigned_integral TSize>
    bool Vector<T, TSize>::Contains( const T tFindValue, const FnFindComparator fnComparator ) const 
    {
        return IndexOf( tFindValue, fnComparator ).bFound;
    }

    template<typename T, std::unsigned_integral TSize>
    inline T* Vector<T, TSize>::Data() { return m_pElements; }

    template<typename T, std::unsigned_integral TSize>
    inline const T* Vector<T, TSize>::DataForReadOnly() const { return const_cast<const T*>( m_pElements ); }

    template<typename T, std::unsigned_integral TSize>
    inline TSize Vector<T, TSize>::Length() const { return m_nLastElement; }

    template<typename T, std::unsigned_integral TSize>
    inline TSize Vector<T, TSize>::ElementsAllocated() const { return m_nAllocatedElements; }
}
