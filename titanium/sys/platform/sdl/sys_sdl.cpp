#include "sys_sdl.hpp"

#include "titanium/logger/logger.hpp"
#include "titanium/sys/platform/sdl/stringify.hpp"

#if _WIN32
#include <windows.h>
#endif

namespace sys::platform::sdl
{
    void Initialise()
    {
        SDL_Init( SDL_INIT_VIDEO );

        logger::Info( "sdl runtime version is %s" ENDL, sys::platform::sdl::StringRuntimeVersion().ConstCStr() );
        logger::Info( "engine was compiled against sdl version %s" ENDL, sys::platform::sdl::StringCompilationVersion().ConstCStr() );
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
        HWND hwnd = sdlPlatWindowInfo.info.win.window;
        HINSTANCE hinstance = sdlPlatWindowInfo.info.win.hinstance;// GetModuleHandle(NULL);

        const WGPUSurfaceDescriptorFromWindowsHWND wgpuSurfaceDescWindows{
            .chain {
                .next = NULL,
                .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND
            },
            .hinstance = hinstance,
            .hwnd = hwnd,
        };

        wgpuSurfaceDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&wgpuSurfaceDescWindows);

#endif // #if _WIN32

        logger::Info( "CreateWebGPUSurfaceForWindow: Windowing backend is %s" ENDL, sys::platform::sdl::SysWMToString( sdlPlatWindowInfo.subsystem ) );
        return wgpuInstanceCreateSurface( wgpuInstance, &wgpuSurfaceDesc );
    }
}
