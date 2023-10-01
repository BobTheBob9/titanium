#include "mem_core.hpp"

#include <stdlib.h>

namespace memory
{
    static int s_nAllocs = 0; // temp: need atomic

    void* alloc(const size_t nBytes, const MemPool *const pMemPool, const std::source_location location)
    {
        (void)pMemPool;
        (void)location;

        // TODO: mempool and logging logic
        s_nAllocs++;
        return ::malloc( nBytes );
    }

    void free(void* pMemoryToFree, const MemPool *const pMemPool, const std::source_location location)
    {
        (void)pMemPool;
        (void)location;

        // TODO: mempool and logging logic
        s_nAllocs--;
        ::free( pMemoryToFree );
    }

    void* realloc( void *const pMemoryToRealloc, const size_t nNewSize, const MemPool *const pMemPool, const std::source_location location )
    {
        (void)pMemPool;
        (void)location;

        // TODO: mempool and logging logic
        return ::realloc( pMemoryToRealloc, nNewSize );
    }

    int GetAllocs() { return s_nAllocs; }
}
