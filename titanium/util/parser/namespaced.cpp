#include "namespaced.hpp"

#include <string.h>

#include "titanium/memory/mem_core.hpp"

/*

Refer to header for function documentation and purposes

*/
namespace util::parser::namespaced
{
    constexpr char NAMESPACE_DELIMITER = ':';

    char ** Parse(const char *const pszInputString, uint32_t *const pnoStrings, memory::MemPool *const pMemPool)
    {
        size_t nLength = strlen(pszInputString) + 1;
        return Parse_n(pszInputString, nLength, pnoStrings, pMemPool);
    }

    char ** Parse_n(const char *const pszInputString, const size_t nBufSize, uint32_t *const pnoStrings, memory::MemPool *const pMemPool)
    {
        // first we need to figure out the number of delimiter chars so we can allocate an array
        for (uint32_t i = 0; pszInputString[i]; i++)
        {
            if (i >= nBufSize)
                return nullptr;
                
            if (pszInputString[i] == NAMESPACE_DELIMITER)
                (*pnoStrings)++;
        }

        // allocate the array
        uint32_t j = 0;
        uint32_t nStringBegin = 0;
        char** ppNamespaces = memory::alloc_nT<char*>(*pnoStrings, pMemPool);

        // actually allocate all our strings and push them to the array
        for (uint32_t i = 0; ; i++)
        {
            if (pszInputString[i] == NAMESPACE_DELIMITER || !pszInputString[i])
            {
                size_t nStringLength = i - nStringBegin;

                // the next member begins at nStringBegin, and ends at i-1
                char* pNamespace = memory::alloc_nT<char>(nStringLength + 1, pMemPool);
                strncpy(pNamespace, pszInputString + nStringBegin, nStringLength);
                pNamespace[nStringLength + 1] = '\0';

                // add to the array of parsed namespaces
                ppNamespaces[j++] = pNamespace;

                // finish parsing if string is done
                if (!pszInputString[i])
                    break;

                // set start of next string
                nStringBegin = i + 1; // after delimeter
            }
        }

        return ppNamespaces;
    }
}



