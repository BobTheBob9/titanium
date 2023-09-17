#include "game_consolecommand.hpp"

#include <libtitanium/logger/logger.hpp>
#include <libtitanium/config/config.hpp>
#include <libtitanium/util/data/staticspan.hpp>

#include <string.h>
#include <ctype.h>

void C_ConsoleAutocomplete( const util::data::Span<char> spszConsoleInput, const util::data::Span<util::data::StringBuf<128>> o_spszAutocompleteItems, void * pCallbackUserData )
{
    (void)pCallbackUserData;

    if ( !*spszConsoleInput.m_pData )
    {
        return;
    }

    util::data::StaticSpan<config::Var *, 10> scvarUntypedVars {};
    {
        util::data::Span<config::Var *> scvarUntypedVarsTemp = scvarUntypedVars.ToSpan(); // temp span to avoid taking address of temp
        config::FindVarsStartingWith( spszConsoleInput.m_pData, &scvarUntypedVarsTemp );
    }

    for ( uint i = 0; i < scvarUntypedVars.Elements() && scvarUntypedVars.m_tData[ i ]; i++ )
    {
        const char *const pszName = scvarUntypedVars.m_tData[i]->szName;
        if ( !util::string::CStringsEqual( spszConsoleInput.m_pData, pszName ) )
        {
            o_spszAutocompleteItems.m_pData[ i ] = pszName; // util::data::StringBuf<128>( "%s = %s", scvarUntypedVars.m_tData[i]->V_GetName(), scvarUntypedVars.m_tData[i]->V_ToString().ToCStr() );
        }

    }
}

void C_ConsoleCommandCompletion( const util::data::Span<char> spszConsoleInput, void * pCallbackUserData )
{
    (void)pCallbackUserData;

    logger::Info( "> %s" ENDL, spszConsoleInput.m_pData );

    util::data::StringBuf<128> szCurrentVar;
    memset( szCurrentVar.m_szStr, '\0', 128 );
    int nCurrentVarLastChar = 0;
    util::data::StringBuf<256> szCurrentValue;
    memset( szCurrentValue.m_szStr, '\0', 128 );
    int nCurrentValueLastChar = 0;
    bool bShouldSetNext = false;

    // parse console input
    for ( uint i = 0; i < spszConsoleInput.m_nElements && spszConsoleInput.m_pData[ i ]; i++ )
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

    config::Var *const pVar = config::FindVar( szCurrentVar.ToConstCStr() );
    if ( pVar )
    {
        if ( bShouldSetNext )
        {
            pVar->setFuncs.fnSetFromString( pVar->pValue, szCurrentValue.ToConstCStr() );
        }

        util::data::StaticSpan<char, 128> sszValue;
        pVar->setFuncs.fnToString( pVar->pValue, sszValue.ToSpan() );
        logger::Info( "%s = %s" ENDL, szCurrentVar.ToConstCStr(), sszValue.m_tData );
    }
    else
    {
        logger::Info( "Unknown var \"%s\"" ENDL, szCurrentVar.ToConstCStr() );
    }
}
