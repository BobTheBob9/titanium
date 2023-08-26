#include "logger.hpp"
#include <stdarg.h>
#include <stdio.h>

#include "libtitanium/util/data/hashmap.hpp"

void logger::Info( const char *const pFmt, ... )
{
    char szBuf[ 4096 ];
    va_list vargs;
    va_start( vargs, pFmt );
    {
        vsprintf( szBuf, pFmt, vargs );
    }
    va_end( vargs );

    printf( "%s", szBuf );
}
