#pragma once

#include <libtitanium/util/numerics.hpp>
#include <libtitanium/util/data/span.hpp>

/*

Functions for more easily performing common string operations

*/
namespace util::string
{
    // TODO: these kind of suck

    int LengthOfCString( const char *const pszString );
    int LengthOfCStringWithTerminator( const char *const pszString );
    int LengthOfSpanString( const util::data::Span<char> spszString );
    void CopyTo( const char *const pszSource, util::data::Span<char> spszDestinationBuffer );
    void ConcatinateTo( const char* const pszSource, util::data::Span<char> spszDestinationBuffer );
    void ToLowercase( char *const pszString );
    void ToUppercase( char *const pszString );
    bool CStringsEqual( const char *const pszFirstString, const char *const pszSecondString );
    bool CStringStartsWith( const char *const pszFirstString, const char *const pszSecondString );
};
