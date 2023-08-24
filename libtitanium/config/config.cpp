#include "config.hpp"

#include <stdio.h>

#include "libtitanium/util/data/span.hpp"
#include "libtitanium/util/data/vector.hpp"

namespace config
{
    ENUM_FLAGS( EFVarSetFlags );
    ENUM_FLAGS( EFVarUsageFlags );

    // TODO: should be a hashmap or something. This is slow to lookup through
    util::data::Vector<IVarAny *> g_vpcvarConfigVarsUserFacing;
    template <typename T> util::data::Vector<Var<T> *> Var<T>::s_vpcvarVarsForType;

    // explicitly declare all types available to the convar templates here, doing this lets us define their implementations in this .cpp file rather than config.hpp
    template Var<bool> * RegisterVar( const char *const pszVarName, const bool tDefaultValue, const EFVarUsageFlags efVarFlags );

    template <typename T> Var<T> * RegisterVar( const char *const pszVarName, const T tDefaultValue, const EFVarUsageFlags efVarFlags )
    {
        Var<T> * pcvarNewVar = memory::alloc_nT<Var<T>>( 1 ); new( pcvarNewVar ) Var<T>; // need to manually initialise anything with a vtable
        pcvarNewVar->efVarFlags = efVarFlags;
        util::string::CopyTo( pszVarName, util::data::Span<char>( sizeof( pcvarNewVar->szName ), pcvarNewVar->szName ) );
        pcvarNewVar->Set( tDefaultValue, EFVarSetFlags::SKIP_ALL_CHECKS );

        Var<T>::s_vpcvarVarsForType.AppendWithAlloc( pcvarNewVar );
        g_vpcvarConfigVarsUserFacing.AppendWithAlloc( pcvarNewVar );

        return pcvarNewVar;
    }

    template <typename T> const char *const Var<T>::V_GetName() const
    {
        return szName;
    }


    template <> EVarSetResult Var<bool>::Set( const bool tValue, const EFVarSetFlags efVarSetFlags ) 
    {
        this->tValue = tValue;
        return EVarSetResult::SUCCESS;
    }

    template <> void Var<bool>::V_SetFromString( const char *const pszValue )
    {
        this->Set( util::string::CStringsEqual( pszValue, "true" ) || util::string::CStringsEqual( pszValue, "1" ), EFVarSetFlags::NONE );
    }

    template <> util::data::StringBuf<128> Var<bool>::V_ToString() const
    {
        return this->tValue ? "true" : "false";
    }


    template <> EVarSetResult Var<i32>::Set( const i32 tValue, const EFVarSetFlags efVarSetFlags )
    {
        this->tValue = tValue;
        return EVarSetResult::SUCCESS;
    }

    template <> void Var<i32>::V_SetFromString( const char *const pszValue )
    {
        this->tValue = strtol( pszValue, nullptr, 10 );
    }

    template <> util::data::StringBuf<128> Var<i32>::V_ToString() const
    {
        util::data::StringBuf<128> r_sBuf;
        sprintf( r_sBuf.ToCStr(), "%i", this->tValue );

        return r_sBuf;
    }


    IVarAny * FindVarUntyped( const char *const pszVarName )
    {
        for ( int i = 0; i < g_vpcvarConfigVarsUserFacing.Length(); i++ )
        {
            if ( util::string::CStringsEqual( pszVarName, ( *g_vpcvarConfigVarsUserFacing.GetAt( i ) )->V_GetName() ) )
            {
                return *g_vpcvarConfigVarsUserFacing.GetAt( i );
            }
        }

        return nullptr;
    }

    //void Register
};
