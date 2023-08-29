#pragma once

#include <source_location>

#include "libtitanium/util/numerics.hpp"

/*

Functions for allocating memory and tracking the allocation of memory

*/  
namespace memory
{
    /*
    
    MALLOC_DEBUG controls whether we should compile in runtime memory debugging features
    We do still include a few monitoring features without MALLOC_DEBUG, but most (high perf cost) shouldn't be
    
    */
    #ifndef MALLOC_DEBUG
        #define MALLOC_DEBUG 1
    #endif  // #ifndef MALLOC_DEBUG


    // TODO: REWORKING!!!

    /*
    
    Debug allocation stuff
    This ALL needs to be blanked for release!
    
    */
    class MemPool
    {
    };

    /*
    
    Wrapper for malloc

    */
    void* alloc(const size_t nBytes, const MemPool *const pMemPool = nullptr, const std::source_location location =
               std::source_location::current());

    /*
    
    Templated malloc function to avoid unnecessary casts

    */
    template<typename T>
    inline T* alloc_T(const size_t nBytes, const MemPool *const pMemPool = nullptr, const std::source_location location =
               std::source_location::current())
    { return static_cast<T*>(alloc(nBytes, pMemPool, location)); }

    /*
    
    Templated malloc function, rather than taking a number of bytes, it allocates sizeof(T) * nElements

    */
    template<typename T>
    inline T* alloc_nT(const size_t nElements, const MemPool *const pMemPool = nullptr, const std::source_location location =
               std::source_location::current())
    { return alloc_T<T>(sizeof(T) * nElements, pMemPool, location); }

    /* 
    
    Wrapped for realloc
    
    */
    void* realloc( void *const pMemoryToRealloc, const size_t nNewSize, const MemPool *const pMemPool = nullptr, const std::source_location location =
               std::source_location::current() );

    /*
    
    Templated realloc function to avoid unnecessary casts and ensure typesafety

    */
    template<typename T>
    inline T* realloc_T( T *const pMemoryToRealloc, const size_t nNewSize, const MemPool *const pMemPool = nullptr, const std::source_location location =
               std::source_location::current() )
    { return static_cast<T*>(realloc(pMemoryToRealloc, nNewSize, pMemPool, location)); }

    /*
    
    Templated realloc function, rather than taking a number of bytes, it allocates sizeof(T) * nElements

    */
    template<typename T>
    inline T* realloc_nT( T *const pMemoryToRealloc, const size_t nElements, const MemPool *const pMemPool = nullptr, const std::source_location location =
               std::source_location::current() )
    { return realloc_T<T>(pMemoryToRealloc, sizeof(T) * nElements, pMemPool, location); }

    /*
    
    Wrapper for free

    */
    void free(void* pMemoryToFree, const MemPool *const pMemPool = nullptr, const std::source_location location =
               std::source_location::current());

    int GetAllocs();
};