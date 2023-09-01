#pragma once

#include <libtitanium/logger/logger.hpp>

namespace assert
{
    /*
    
    Throw an error if condition is not met, even in release!

    */
    inline void Release( const bool bCondition, const char * const pMessageFormat = nullptr, ... )
    {
        // TODO: error
        if ( !bCondition ) [[ unlikely ]] 
        {
            logger::Info( "assertion failed %s" ENDL, pMessageFormat );
        }
    }


    /*
    
    Throw an error if condition is not met, only in debug/development!
    
    */
    inline void Debug( const bool bCondition, const char * const pMessageFormat = nullptr, ... )
    {
        // TODO: error
        if ( !bCondition ) [[ unlikely ]] {}
    }
};
