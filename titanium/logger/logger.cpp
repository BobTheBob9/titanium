#include "logger.hpp"
#include <stdarg.h>
#include <stdio.h>

void logger::Info( const char *const pFmt, ... )
{
    va_list vargs;
    va_start( vargs, pFmt );

    char szbuf[ 4096 ];
    vsprintf( szbuf, pFmt, vargs );

    va_end( vargs );

    printf( "%s", szbuf );
}