#include "game_consolecommand.hpp"

#include <libtitanium/logger/logger.hpp>
#include <libtitanium/config/config.hpp>
#include <libtitanium/util/data/staticspan.hpp>

#include <string.h>
#include <ctype.h>

void C_ConsoleAutocomplete( const util::data::Span<char> spszConsoleInput, const util::data::Span<util::data::StringBuf<128>> o_spszAutocompleteItems, void * pCallbackUserData )
{
    util::data::StaticSpan<config::IVarAny *, 10> scvarUntypedVars {};
    {
        util::data::Span<config::IVarAny *> scvarUntypedVarsTemp = scvarUntypedVars.ToSpan(); // temp span to avoid taking address of temp
        config::FindVarsStartingWith( spszConsoleInput.m_pData, &scvarUntypedVarsTemp );
    }

    for ( int i = 0; i < scvarUntypedVars.Elements() && scvarUntypedVars.m_tData[ i ]; i++ )
    {
        const char *const pszName = scvarUntypedVars.m_tData[i]->V_GetName();
        if ( !util::string::CStringsEqual( spszConsoleInput.m_pData, pszName ) )
        {
            o_spszAutocompleteItems.m_pData[ i ] = scvarUntypedVars.m_tData[i]->V_GetName(); // util::data::StringBuf<128>( "%s = %s", scvarUntypedVars.m_tData[i]->V_GetName(), scvarUntypedVars.m_tData[i]->V_ToString().ToCStr() );
        }

    }
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
