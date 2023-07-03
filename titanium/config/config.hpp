#pragma once

namespace config
{
    struct ConfigVar
    {
        enum class EFUsageFlags
        {
            NONE = 0,

            READONLY = ( 1 << 0 ), // The config var can be read, but can't be changed by users
            STARTUP = ( 1 << 1 ), // The config var can only be set during startup (e.g. through startup args, or cfg on startup)
        };

        struct Value
        {

        };

        const EFUsageFlags m_efUsageFlags;
    };

    enum EFVarSetFlags
    {
        NONE = 0,

        ALLOW_STARTUP = ( 1 << 0 ), // This call can set vars with ConfigVar::EFUsageFlags::STARTUP
        USER_LOGS = ( 1 << 1 ), // This call can log informati
    };

    //void ConfigVar_Set( ConfigVar *const pConfigVar, , const EFVarSetFlags efVarSetFlags );
};
