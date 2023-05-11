#pragma once

#include <source_location>

#include "titanium/util/numerics.hpp"

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

    /*
    
    Debug allocation stuff
    This ALL needs to be blanked for release!
    
    */
    class MemPool
    {
        // no data, unless we have memory debugging on
        #if MALLOC_DEBUG
            char * m_pszName;
            char * m_pszFileName;
            char * m_pszFunctionName;
            u32 m_nLine;
            u32 m_nColumn;

            const MemPool * m_pParent = nullptr;

            void SetLocation(const std::source_location *const location);
        #endif // #if MALLOC_DEBUG
    public:
        /*
        
        "Normal" MemPool constructor: creates a new named mempool for continuous usage over the lifetime of a system

        */
        MemPool(const char *const pszName, const std::source_location location = std::source_location::current());

        /*
        
        Child MemPool constructor: primarily for passing to util functions that allocate memory, so we can still track allocations for them
                                   creates a new mempool, but parent should be used for almost all getters/setters
        
        */
        MemPool(const MemPool *const pParentPool, const std::source_location location = std::source_location::current());

        inline const char * GetName() const;
        inline const char * GetFileName() const;

        // these should only really be used when a child pool is expected! These won't be useful values on non-children!
        inline const char * GetFunctionName() const;
        inline u32 GetLine() const;
        inline u32 GetColumn() const;
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
};