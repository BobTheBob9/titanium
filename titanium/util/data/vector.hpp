#pragma once

#include <concepts>
#include <string.h>

#include "titanium/memory/mem_core.hpp"
#include "titanium/util/numerics.hpp"
#include "titanium/util/data/span.hpp"
#include "titanium/util/assert.hpp"

#include "titanium/dev/tests.hpp"

namespace util::data
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
        inline TSize LastIndex() const;
    };
}

#include "vector_impl.inl"
#if USE_TESTS
    #include "vector_tests.inl"
#endif // #if USE_TESTS
