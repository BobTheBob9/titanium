#pragma once

#include <type_traits>

#include <libtitanium/util/numerics.hpp>
#include <libtitanium/util/enum_flags.hpp>
#include <libtitanium/util/data/vector.hpp>
#include <libtitanium/util/data/stringbuf.hpp>
#include <libtitanium/util/string.hpp>

namespace config
{
    enum class EFVarSetFlags
    {
        NONE = 0,

        NON_CODE_SOURCE = ( 1 << 0 ), // This call isn't code-driven, it's from a user or game provided call
        ALLOW_SET_STARTUP = ( 1 << 1 ), // This call can set vars with ConfigVar::EFUsageFlags::STARTUP

        SKIP_ALL_CHECKS = ( 1 << 2 )
    };
    ENUM_FLAGS_DECLARE( EFVarSetFlags );
    
    enum class EFVarUsageFlags
    {
        NONE = 0,

        READONLY = ( 1 << 0 ), // The config var can be read, but can't be changed by users
        STARTUP = ( 1 << 1 ), // The config var can only be set during startup (e.g. through startup args, or cfg on startup)
        TEMPORARY = ( 1 << 2 ), // Hint that the config var is likely to be destroyed during the program's runtime other than shutdown (good for code-created cvars!!)
    };
    ENUM_FLAGS_DECLARE( EFVarUsageFlags );

    enum class EVarSetResult
    {
        SUCCESS,

        INVALID_READONLY,
        INVALID_STARTUP,
    };

    // interface for querying untyped config vars
    // NOTE: generally we don't like inheritance, but in this case i'd say it's reasonable
    class IVarAny
    {
    public:
        virtual const char *const V_GetName() const = 0;

        virtual void V_SetFromString( const char *const pszValue ) = 0;
        virtual util::data::StringBuf<128> V_ToString() const = 0;
    };

    template <typename T>
    struct Var : public IVarAny
    {
        static util::data::Vector<Var<T> *> s_vpcvarVarsForType;

        EFVarUsageFlags efVarFlags;

        char szName[128];
        T tValue;

        const char *const ToString() const;
        EVarSetResult Set( const T tValue, const EFVarSetFlags efVarSetFlags );

        const char *const V_GetName() const override final;
        void V_SetFromString( const char *const pszValue ) override final;
        util::data::StringBuf<128> V_ToString() const override final;
    };

    template <typename T> Var<T> * RegisterVar( const char *const pszVarName, const T tDefaultValue, const EFVarUsageFlags efVarFlags );

    template <typename T> Var<T> * FindVar( const char *const pszVarName );
    IVarAny * FindVarUntyped( const char *const pszVarName );

    void FindVarsStartingWith( const char *const pszVarSearchString, util::data::Span<IVarAny *> * o_pspcvarVars );

    void FreeVars();
};
