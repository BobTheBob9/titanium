#include "logger.hpp"
#include <stdarg.h>
#include <stdio.h>

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
