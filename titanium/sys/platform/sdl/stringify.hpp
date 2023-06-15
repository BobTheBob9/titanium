#pragma once

#include "titanium/util/data/stringbuf.hpp"

namespace sys::platform::sdl
{
    const ::utils::data::StringBuf<32> VersionToString( const SDL_version sdlVersion )
    {
        return ::utils::data::StringBuf<32>( "%i.%i.%i", sdlVersion.major, sdlVersion.minor, sdlVersion.patch );
    }

    const ::utils::data::StringBuf<32> StringRuntimeVersion()
    {
        SDL_version sdlRuntimeVersion; 
        SDL_GetVersion( &sdlRuntimeVersion );

        return VersionToString( sdlRuntimeVersion );
    }

    const ::utils::data::StringBuf<32> StringCompilationVersion()
    {
        SDL_version sdlCompiledVersion; 
        SDL_VERSION( &sdlCompiledVersion );

        return VersionToString( sdlCompiledVersion );
    }

    const char *const SysWMToString( const SDL_SYSWM_TYPE esdlWMType )
    {
        switch ( esdlWMType )
        {
            case SDL_SYSWM_UNKNOWN:
            {
                return "SDL_SYSWM_UNKNOWN";
            }

            case SDL_SYSWM_X11:
            {
                return "Linux: X11";
            }

            case SDL_SYSWM_WAYLAND:
            {
                return "Linux: Wayland";
            } 

            case SDL_SYSWM_WINDOWS:
            {
                return "Windows";
            }

            case SDL_SYSWM_COCOA:
            {
                return "MacOS: Cocoa";
            }
            
            default:
            {
                return "Unsupported";
            }
        }
    }
}