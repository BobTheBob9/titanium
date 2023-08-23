#include "sys_sdl.hpp"

#include "libtitanium/logger/logger.hpp"
#include "libtitanium/sys/platform/sdl/stringify.hpp"

namespace sys::platform::sdl
{
    void Initialise()
    {
        SDL_Init( SDL_INIT_VIDEO );

        logger::Info( "sdl runtime version is %s" ENDL, sys::platform::sdl::StringRuntimeVersion().ToConstCStr() );
        logger::Info( "engine was compiled against sdl version %s" ENDL, sys::platform::sdl::StringCompilationVersion().ToConstCStr() );
    }

    WGPUSurface CreateWGPUSurfaceForWindow( SDL_Window *const psdlWindow, const WGPUInstance wgpuInstance )
    {        
        SDL_SysWMinfo sdlPlatWindowInfo;
        SDL_VERSION( &sdlPlatWindowInfo.version );
        SDL_GetWindowWMInfo( psdlWindow, &sdlPlatWindowInfo );

        // grab a wgpu surface from SDL for our current platform, as SDL2 doesn't natively support wgpu
        // TODO: no win32 or macos support yet
        WGPUSurfaceDescriptor wgpuSurfaceDesc; // we're filling this out to pass to wgpuInstanceCreateSurface later

#ifdef linux
        // declare structs here so we don't go out of scope etc
        const WGPUSurfaceDescriptorFromXlibWindow wgpuSurfaceDescX11 {
            .chain { .sType = WGPUSType_SurfaceDescriptorFromXlibWindow },

            .display = sdlPlatWindowInfo.info.x11.display,
            .window = (uint32_t)sdlPlatWindowInfo.info.x11.window
        };

        const WGPUSurfaceDescriptorFromWaylandSurface wgpuSurfaceDescWayland {
            .chain { .sType = WGPUSType_SurfaceDescriptorFromWaylandSurface },

            .display = sdlPlatWindowInfo.info.wl.display,
            .surface = sdlPlatWindowInfo.info.wl.surface
        };

        // linux: x11/wayland
        if ( sdlPlatWindowInfo.subsystem == SDL_SYSWM_X11 )
        {
            wgpuSurfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct *>( &wgpuSurfaceDescX11 );
        }
        else if ( sdlPlatWindowInfo.subsystem == SDL_SYSWM_WAYLAND )
        {
            wgpuSurfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct *>( &wgpuSurfaceDescWayland );
        }
#endif // #ifdef linux

#if _WIN32
        #error "No win32 support, atm"
            
#endif // #if _WIN32

        logger::Info( "CreateWebGPUSurfaceForWindow: Windowing backend is %s" ENDL, sys::platform::sdl::SysWMToString( sdlPlatWindowInfo.subsystem ) );
        return wgpuInstanceCreateSurface( wgpuInstance, &wgpuSurfaceDesc );
    }

    util::maths::Vec2<u32> GetWindowSizeVector( SDL_Window *const psdlWindow )
    {
        int nWindowWidth, nWindowHeight;
        SDL_GetWindowSize( psdlWindow, &nWindowWidth, &nWindowHeight );
        return util::maths::Vec2<u32> { .x = static_cast<u32>( nWindowWidth ), .y = static_cast<u32>( nWindowHeight ) };
    }
}
