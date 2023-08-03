#pragma once

#include "titanium/util/numerics.hpp"

#include <stdarg.h>
#include <string.h>

namespace util::data
{
    template <u64 TMaxSize>
    struct StringBuf
    {
        char m_szStr[ TMaxSize ];

        StringBuf();
        StringBuf( const char *const pszFmt, ... );

        char * ToCStr();
        const char * ToConstCStr() const;
        operator char * ();
        operator const char * () const;
    };
}

#include "stringbuf_impl.inl"
