#include "game_consolecommand.hpp"

#include <libtitanium/util/string.hpp>
#include <libtitanium/logger/logger.hpp>
#include <libtitanium/config/config.hpp>
#include <libtitanium/util/static_array.hpp>

#include <string.h>
#include <ctype.h>

void C_ConsoleAutocomplete( const util::data::Span<char> spszConsoleInput, const util::data::Span<util::data::StringBuf<128>> o_spszAutocompleteItems, void * pCallbackUserData )
{
    (void)pCallbackUserData;

    if ( !*spszConsoleInput.m_pData )
    {
        return;
    }

    config::Var * pcvarUntypedVars[10] {};
    config::FindVarsStartingWith( spszConsoleInput.m_pData, util::StaticArray_ToSpan( pcvarUntypedVars ) );

    for ( uint i = 0; i < util::StaticArray_Length( pcvarUntypedVars ) && pcvarUntypedVars[ i ]; i++ )
    {
        const char *const pszName = pcvarUntypedVars[i]->szName;
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
            pVar->bDirty = true;
        }

        char szValue[ 128 ];
        pVar->setFuncs.fnToString( pVar->pValue, util::StaticArray_ToSpan( szValue ) );
        logger::Info( "%s = %s" ENDL, szCurrentVar.ToConstCStr(), szValue );
    }
    else
    {
        logger::Info( "Unknown var \"%s\"" ENDL, szCurrentVar.ToConstCStr() );
    }
}
