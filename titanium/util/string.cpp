#include "string.hpp"

#include <string.h>
#include <functional>

#include "titanium/memory/mem_core.hpp"

/*

Refer to header for function documentation and purposes

*/
namespace util::string
{ 
    char * Clone(const char * const pszToClone)
    {
        return Clone_n(pszToClone, __SIZE_MAX__ );
    }

    char * Clone_n(const char * const pszToClone, const size_t nBufSize)
    {
        size_t nStrSize = strnlen(pszToClone, nBufSize);
        if (nStrSize == nBufSize) // overrun!
            return nullptr;

        char *const pszBuf = memory::alloc_T<char>(nStrSize + 1);
        strcpy(pszBuf, pszToClone);
        pszBuf[nStrSize] = '\0';

        return pszBuf;
    }

    /*u32 Hash32(const char * const pszToHash)
    {
    }

    u32 Hash32_n(const char * const pszToHash, const size_t nBufSize)
    {
    }

    u64 Hash64(const char * const pszToHash)
    {
    }

    u64 Hash64_n(const char * const pszToHash, const size_t nBufSize)
    {
    }*/
};