#include "mem_core.hpp"

#include <stdlib.h>
#include <atomic>

namespace memory
{
#if HAS_MEM_DEBUG
    static int s_nAllocs = 0; // temp: need atomic
#endif // #if HAS_MEM_DEBUG

    void* alloc(const size_t nBytes, const MemPool *const pMemPool, const std::source_location location)
    {
        (void)pMemPool;
        (void)location;

#if HAS_MEM_DEBUG
        // TODO: mempool and logging logic
        s_nAllocs++;
#endif // #if HAS_MEM_DEBUG
        return ::malloc( nBytes );
    }

    void free(void* pMemoryToFree, const MemPool *const pMemPool, const std::source_location location)
    {
        (void)pMemPool;
        (void)location;

#if HAS_MEM_DEBUG
        // TODO: mempool and logging logic
        s_nAllocs--;
#endif // #if HAS_MEM_DEBUG
        ::free( pMemoryToFree );
    }

    void* realloc( void *const pMemoryToRealloc, const size_t nNewSize, const MemPool *const pMemPool, const std::source_location location )
    {
        (void)pMemPool;
        (void)location;

        // TODO: mempool and logging logic
        return ::realloc( pMemoryToRealloc, nNewSize );
    }

    void * externalMalloc( size_t size ) { return ::memory::alloc( size ); }
    void * externalCalloc( size_t nmemb, size_t size )
    {
#if HAS_MEM_DEBUG
        // TODO: mempool and logging logic
        s_nAllocs++;
#endif // #if HAS_MEM_DEBUG
        return ::calloc( nmemb, size );
    }
    void * externalRealloc( void * ptr, size_t size ) { return realloc( ptr, size ); }
    void * externalReallocarray( void * ptr, size_t nmemb, size_t size ) { return ::reallocarray( ptr, nmemb, size ); }
    void externalFree( void * ptr ) { return ::memory::free( ptr ); }

    int GetAllocs()
    {
#if HAS_MEM_DEBUG
        return s_nAllocs;
#else // #if HAS_MEM_DEBUG
        return 0;
#endif // #else // #if HAS_MEM_DEBUG
    }
}
