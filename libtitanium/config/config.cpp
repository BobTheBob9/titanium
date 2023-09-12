#include "config.hpp"
#include "util/data/staticspan.hpp"
#include "util/data/stringbuf.hpp"
#include "util/string.hpp"

#include <stdio.h>

#include <libtitanium/memory/mem_core.hpp>
#include <libtitanium/util/data/span.hpp>
#include <libtitanium/util/data/vector.hpp>

namespace config
{
    ENUM_FLAGS( EFVarSetFlags );
    ENUM_FLAGS( EFVarUsageFlags );

    static util::data::Vector<Var> s_vcvarVars;

    void VarBool_ToString( const void *const pCvarPointer, util::data::Span<char> o_spszOutputBuffer )
    {
        util::string::CopyTo( *static_cast<const bool *const>( pCvarPointer ) ? "true" : "false", o_spszOutputBuffer );
    }

    void VarBool_SuggestValues( const void *const pCvarPointer, const char *const pszIncompleteValue, util::data::Span<util::data::StringBuf<32>> o_sspszOutputBuffer )
    {
        constexpr const char *const BOOL_VALUES[] { "true", "false" };

        int nOutputIndex = 0;
        for ( int i = 0; i < sizeof( BOOL_VALUES ) / sizeof( char * ) && nOutputIndex < o_sspszOutputBuffer.m_nElements; i++ )
        {
            if ( util::string::CStringStartsWith( BOOL_VALUES[ i ], pszIncompleteValue ) )
            {
                o_sspszOutputBuffer.m_pData[ nOutputIndex++ ] = BOOL_VALUES[ i ];
            }
        }
    }

    void VarBool_SetFromString( void *const pCvarPointer, const char *const pszValue )
    {
        util::data::StringBuf<32> pszLowercaseValue = pszValue;
        util::string::ToLowercase( pszLowercaseValue.m_szStr );

        *static_cast<bool* const>( pCvarPointer ) = util::string::CStringsEqual( pszLowercaseValue, "true" ) || atoi( pszLowercaseValue ) > 0;
    }



    Var RegisterVar( const char *const pszName, const EFVarUsageFlags efUsage, Var::SetFuncs setFuncs, void *const pValue )
    {
        Var cvar {
            .szName {},
            .setFuncs = setFuncs,
            .pValue = pValue
        };

        util::string::CopyTo( pszName, util::data::Span<char>( sizeof( cvar.szName ), cvar.szName ) );

        s_vcvarVars.AppendWithAlloc( cvar );
        return cvar;
    }

    Var * FindVar( const char *const pszVarName )
    {
        for ( int i = 0; i < s_vcvarVars.Length(); i++ )
        {
            if ( util::string::CStringsEqual( pszVarName, s_vcvarVars.GetAt( i )->szName ) )
            {
                return s_vcvarVars.GetAt( i );
            }
        }

        return nullptr;
    }

    void FindVarsStartingWith( const char *const pszVarSearchString, util::data::Span<Var *> * o_pspcvarVars )
    {
        int nFoundVars = 0;
        for ( int i = 0; i < s_vcvarVars.Length() && nFoundVars < o_pspcvarVars->m_nElements; i++ )
        {
            if ( util::string::CStringStartsWith( s_vcvarVars.GetAt( i )->szName, pszVarSearchString ) )
            {
                o_pspcvarVars->m_pData[ nFoundVars++ ] = s_vcvarVars.GetAt( i );
            }
        }
    }

    // TODO: this sucks, and doesn't free everything
    void StaticFree()
    {
        s_vcvarVars.SetAllocated( 0 );
    }
};
