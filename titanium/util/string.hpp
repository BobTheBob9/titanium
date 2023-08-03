#pragma once

#include "titanium/util/numerics.hpp"

/*

Functions for more easily performing common string operations

*/
namespace util::string
{
    // NOTE: these work because all lowercase ascii chars are +32 from uppercase
    void ToLowercase( char *const pszStringToConvert )
    {
        for ( char * pszStringIterator = pszStringToConvert; *pszStringIterator; pszStringIterator++ )
        {
            *pszStringIterator += 32 * ( *pszStringIterator >= 'A' && *pszStringIterator <= 'Z' );
        }
    }

    void ToUppercase( char *const pszStringToConvert )
    {
        for ( char * pszStringIterator = pszStringToConvert; *pszStringIterator; pszStringIterator++ )
        {
            *pszStringIterator -= 32 * ( *pszStringIterator >= 'a' && *pszStringIterator <= 'z' );
        }
    }
};