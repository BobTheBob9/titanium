#include "config.hpp"

#include <stdio.h>

#include <libtitanium/memory/mem_core.hpp>
#include <libtitanium/util/data/span.hpp>
#include <libtitanium/util/data/vector.hpp>

namespace config
{
    ENUM_FLAGS( EFVarSetFlags );
    ENUM_FLAGS( EFVarUsageFlags );

    // TODO: should be a hashmap or something. This is slow to lookup through
    static util::data::Vector<IVarAny *> s_vpcvarConfigVarsUntyped;
    template <typename T> util::data::Vector<Var<T> *> Var<T>::s_vpcvarVarsForType;

    // explicitly declare all types available to the convar templates here, doing this lets us define their implementations in this .cpp file rather than config.hpp
    template Var<bool> * RegisterVar( const char *const pszVarName, const bool tDefaultValue, const EFVarUsageFlags efVarFlags );
    template Var<i32> * RegisterVar( const char *const pszVarName, const i32 tDefaultValue, const EFVarUsageFlags efVarFlags );

    template <typename T> Var<T> * RegisterVar( const char *const pszVarName, const T tDefaultValue, const EFVarUsageFlags efVarFlags )
    {
        // TODO: this sucks and is bad, need to reexamine whether we actually need inheritance here
        // need to manually initialise anything with a vtable (not sure why placement new is needed? idk why *pcvarNewVar = Var<T>() doesnt just work for this)
        Var<T> * pcvarNewVar = new( memory::alloc_nT<Var<T>>( 1 ) ) Var<T>;
        pcvarNewVar->efVarFlags = efVarFlags;
        util::string::CopyTo( pszVarName, util::data::Span<char>( sizeof( pcvarNewVar->szName ), pcvarNewVar->szName ) );
        pcvarNewVar->Set( tDefaultValue, EFVarSetFlags::SKIP_ALL_CHECKS );

        Var<T>::s_vpcvarVarsForType.AppendWithAlloc( pcvarNewVar );
        s_vpcvarConfigVarsUntyped.AppendWithAlloc( pcvarNewVar );

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
        for ( int i = 0; i < s_vpcvarConfigVarsUntyped.Length(); i++ )
        {
            if ( util::string::CStringsEqual( pszVarName, ( *s_vpcvarConfigVarsUntyped.GetAt( i ) )->V_GetName() ) )
            {
                return *s_vpcvarConfigVarsUntyped.GetAt( i );
            }
        }

        return nullptr;
    }

    void FindVarsStartingWith( const char *const pszVarSearchString, util::data::Span<IVarAny *> * o_pspcvarVars )
    {
        int nFoundVars = 0;
        for ( int i = 0; i < s_vpcvarConfigVarsUntyped.Length() && nFoundVars < o_pspcvarVars->m_nElements; i++ )
        {
            if ( util::string::CStringStartsWith( ( *s_vpcvarConfigVarsUntyped.GetAt( i ) )->V_GetName(), pszVarSearchString ) )
            {
                o_pspcvarVars->m_pData[ nFoundVars++ ] = *s_vpcvarConfigVarsUntyped.GetAt( i );
            }
        }
    }

    // TODO: this sucks, and doesn't free everything
    void FreeVars()
    {
        for ( int i = 0; i < s_vpcvarConfigVarsUntyped.Length(); i++ )
        {
            memory::free( *s_vpcvarConfigVarsUntyped.GetAt( i ) );
        }
    }
};
