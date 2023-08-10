#pragma once

#include "titanium/util/numerics.hpp"

/*

Functions for more easily performing common string operations

*/
namespace util::string
{
    char * AllocForSIMD( const size_t nChars );

    int Length( char *const pszStringToCheck );
    int Length_SSE( char *const pa16szStringToCheck );

    void CopyTo( const char *const pszSource, char *const pszDestinationBuffer );
    void CopyTo_SSE( const char *const pa16szSource, char *const pa16szDestinationBuffer );

    void ToLowercase( char *const pszStringToConvert );
    void ToLowercase_SSE( char *const pa16szStringToConvert );

    void ToUppercase( char *const pszStringToConvert );
    void ToUppercase_SSE( char *const pa16szStringToConvert );
};
