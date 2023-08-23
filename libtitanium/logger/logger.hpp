#pragma once

#define ENDL "\n"

namespace logger
{
    enum class eLogLevel
    {
        INFO,
        WARN,
        ERROR,
        ERROR_FATAL
    };

    void Log( const eLogLevel eLevel, const char *const pFmt, ... );

    void Info( const char *const pFmt, ... );
    void Warn( const char *const pFmt, ... );
    void Error( const char *const pFmt, ... );
};

#define LOG_CALL( func ) logger::Info( "Call: " #func " in " __FILE__ ENDL ); func

#define LOG_SCOPE( ... )  