#include "mem_core.hpp"

#include <stdlib.h>

#include "titanium/util/string.hpp"

namespace memory
{
    void* alloc(const size_t nBytes, const MemPool *const pMemPool, const std::source_location location)
    {
        // TODO: mempool and logging logic
        return ::malloc(nBytes);
    }

    void free(void* pMemoryToFree, const MemPool *const pMemPool, const std::source_location location)
    {
        // TODO: mempool and logging logic
        ::free(pMemoryToFree);
    }

    void* realloc( void *const pMemoryToRealloc, const size_t nNewSize, const MemPool *const pMemPool, const std::source_location location )
    {
        // TODO: mempool and logging logic
        return ::realloc( pMemoryToRealloc, nNewSize );
    }
}
