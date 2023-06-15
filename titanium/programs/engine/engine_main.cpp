// systems that need initialising
#include "titanium/sys/platform/sdl/sys_sdl.hpp"
#include "titanium/config/config.hpp"
#include "titanium/filesystem/fs_api.hpp"
#include "titanium/jobsystem/jobs_api.hpp"
#include "titanium/logger/logger.hpp"
#include "titanium/renderer/renderer_api.hpp"

int main()
{
    logger::Info( "wow we're running!" ENDL );
    //filesystem::Initialise( "" );
    //jobsystem::Initialise();

    sys::platform::sdl::Initialise();
    SDL_Window *const psdlWindow = SDL_CreateWindow( "Titanium - SDL + WGPU", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1600, 900, SDL_WINDOW_SHOWN ); // TODO: do we need SDL_WINDOW_VULKAN?

    renderer::TitaniumRendererState rendererState {};
    LOG_CALL( renderer::Initialise( &rendererState, psdlWindow ) );

    bool bRunEngine = true;
    while ( bRunEngine )
    {
        SDL_Event sdlEvent;
        while ( SDL_PollEvent( &sdlEvent ) )
        {
            if ( sdlEvent.type == SDL_QUIT )
            {
                bRunEngine = false;
                break;
            }
        }

        renderer::Frame( &rendererState );
    }
}
