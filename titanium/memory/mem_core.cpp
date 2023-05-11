#include "mem_core.hpp"

#include <stdlib.h>

#include "titanium/util/string.hpp"

namespace memory
{
    constexpr const char *const pszNO_MALLOC_DEBUG_PLACEHOLDER = "MALLOC_DEBUG = false!";
    constexpr int nNO_MALLOC_DEBUG_PLACEHOLDER = 0;

    inline const char * MemPool::GetName() const
    {
        #if MALLOC_DEBUG
            if (m_pParent)
                return m_pParent->GetName();
            else
                return m_pszName;
        #else
            return pszNO_MALLOC_DEBUG_PLACEHOLDER;
        #endif // #if MALLOC_DEBUG
    }

    inline const char * MemPool::GetFileName() const
    {
        #if MALLOC_DEBUG
            return m_pszFileName;
        #else
            return pszNO_MALLOC_DEBUG_PLACEHOLDER;
        #endif // #if MALLOC_DEBUG
    }

    inline const char * MemPool::GetFunctionName() const 
    {
        #if MALLOC_DEBUG
            return m_pszFunctionName;
        #else
            return pszNO_MALLOC_DEBUG_PLACEHOLDER;
        #endif // #if MALLOC_DEBUG
    }

    inline u32 MemPool::GetLine() const
    {
        #if MALLOC_DEBUG
            return m_nLine;
        #else
            return nNO_MALLOC_DEBUG_PLACEHOLDER;
        #endif // #if MALLOC_DEBUG
    }

    inline u32 MemPool::GetColumn() const
    {
        #if MALLOC_DEBUG
            return m_nColumn;
        #else
            return nNO_MALLOC_DEBUG_PLACEHOLDER;
        #endif // #if MALLOC_DEBUG
    }

    #if MALLOC_DEBUG
        void MemPool::SetLocation(const std::source_location *const location)
        {
            m_pszFileName = util::string::Clone(location->file_name());
            m_pszFunctionName = util::string::Clone(location->function_name());
            m_nLine = location->line();
            m_nColumn = location->column();
        }
    #endif // #if MALLOC_DEBUG

    MemPool::MemPool(const char *const pszName, const std::source_location location)
    {
        #if MALLOC_DEBUG
            m_pszName = util::string::Clone(pszName);
            SetLocation(&location);
        #endif // #if MALLOC_DEBUG
    }

    MemPool::MemPool(const MemPool *const pMemPool, const std::source_location location)
    {
        #if MALLOC_DEBUG
            m_pParent = pMemPool;
            SetLocation(&location);
        #endif // #if MALLOC_DEBUG
    }

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
