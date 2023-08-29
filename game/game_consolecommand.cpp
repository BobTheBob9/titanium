#include "game_consolecommand.hpp"

#include "libtitanium/util/data/stringbuf.hpp"
#include "libtitanium/logger/logger.hpp"
#include "libtitanium/config/config.hpp"

#include <string.h>
#include <ctype.h>

util::data::StringBuf<128> C_ConsoleAutocomplete( const util::data::Span<char> spszConsoleInput, void * pUserData )
{
    return util::data::StringBuf<128>();
}

void C_ConsoleCommandCompletion( const util::data::Span<char> spszConsoleInput, void * pCallbackUserData )
{
    logger::Info( "> %s" ENDL, spszConsoleInput.m_pData );

    util::data::StringBuf<128> szCurrentVar;
    memset( szCurrentVar.m_szStr, '\0', 128 );
    int nCurrentVarLastChar = 0;
    util::data::StringBuf<256> szCurrentValue;
    memset( szCurrentValue.m_szStr, '\0', 128 );
    int nCurrentValueLastChar = 0;
    bool bShouldSetNext = false;

    // parse console input
    for ( int i = 0; i < spszConsoleInput.m_nElements && spszConsoleInput.m_pData[ i ]; i++ )
    {
        const char cCurrentChar = spszConsoleInput.m_pData[ i ];
        if ( !isspace( cCurrentChar ) )
        {
            if ( cCurrentChar == ';' )
            {
                // next command
            }
            else if ( cCurrentChar == '=') // set
            {
                bShouldSetNext = true;
            }
            else
            {
                if ( bShouldSetNext )
                {
                    if ( nCurrentValueLastChar < 255 )
                    {
                        szCurrentValue.m_szStr[ nCurrentValueLastChar++ ] = cCurrentChar;
                    }
                }
                else
                {
                    if ( nCurrentVarLastChar < 127 )
                    {
                        szCurrentVar.m_szStr[ nCurrentVarLastChar++ ] = cCurrentChar;
                    }
                }
            }
        }
    }

    config::IVarAny *const pVarAny = config::FindVarUntyped( szCurrentVar.ToConstCStr() );
    if ( pVarAny )
    {
        if ( bShouldSetNext )
        {
            pVarAny->V_SetFromString( szCurrentValue.ToConstCStr() );
        }

        logger::Info( "%s = %s" ENDL, pVarAny->V_GetName(), pVarAny->V_ToString().ToConstCStr() );
    }
    else
    {
        logger::Info( "Unknown var \"%s\"" ENDL, szCurrentVar.ToConstCStr() );
    }
}
