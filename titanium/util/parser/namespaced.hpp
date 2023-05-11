#pragma once

#include "titanium/util/numerics.hpp"
#include "titanium/memory/mem_core.hpp" 

/*

Functions for parsing "namespaced" strings for variable/asset names
For example, this parses "filesystem:async:threads" into [ "filesystem", "async", "threads" ]

*/
namespace util::parser::namespaced
{
    /*
    
    Parse a namespaced string into its individual members
    Returns a an array of null-terminated strings, with the length of the array stored in pnoStrings
    Returns nullptr if the namespaced string cannot be parsed
    MEM: Caller is responsible for freeing memory allocated by this function! (Both the array itself, and the string members within it)

    */
    char ** Parse(const char *const pszInputString, u32 *const pnoStrings, memory::MemPool *const pMemPool = nullptr );

    /*
    
    Refer to util::parser::namespaced::Parse
    Operates identically, but takes a max size, parsing will fail if the buffer being read is larger than the max size

    */
    char ** Parse_n(const char *const pszInputString, const size_t nBufSize, u32 *const pnoStrings, memory::MemPool *const pMemPool = nullptr );
};