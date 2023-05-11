#pragma once

#include "titanium/util/numerics.hpp"

/*

Functions for more easily performing common string operations

*/
namespace util::string
{
    /*
    
    Creates a new string buffer, copying the input string
    Returns the new string buffer
    Returns nullptr if the string could not be cloned
    MEM: Caller is responsible for freeing memory allocated by this function!
    
    */
    char * Clone(const char * const pszToClone);

    /*
    
    Refer to util::string::Clone
    Operates identically, but takes a max size, cloning will fail if the buffer being read is larger than the max size
    
    */
    char * Clone_n(const char * const pszToClone, const size_t nBufSize);
};