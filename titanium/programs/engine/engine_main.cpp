#include "extern/imgui/imgui.h"
#include "extern/imgui/imgui_impl_sdl2.h"
#include "extern/imgui/imgui_impl_wgpu.h"

#include "titanium/util/commandline.hpp"

// systems that need initialising
#include "titanium/sys/platform/sdl/sys_sdl.hpp"
#include "titanium/config/config.hpp"
#include "titanium/filesystem/fs_api.hpp"
#include "titanium/jobsystem/jobs_api.hpp"
#include "titanium/logger/logger.hpp"
#include "titanium/renderer/renderer_api.hpp"

#include "titanium/dev/tests.hpp"

int main( const int nArgs, const char *const *const ppszArgs )
{
    logger::Info( "Hello world! From Titanium!!!" ENDL );

    // grab commandline args
    util::commandline::CommandArgs caCommandLine {};
    util::commandline::CreateFromSystemWithAlloc( &caCommandLine, nArgs, ppszArgs );

    //filesystem::Initialise( "" );
    //jobsystem::Initialise();

    sys::platform::sdl::Initialise();
    SDL_Window *const psdlWindow = SDL_CreateWindow( "Titanium - SDL + WGPU", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1600, 900, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN ); // TODO: do we need SDL_WINDOW_VULKAN/whatever api?

    // initialise imgui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = 1600;
    io.DisplaySize.y = 900;
    ImGui_ImplSDL2_InitForVulkan( psdlWindow ); // TODO: looking at the ImGui_ImplSDL2_InitFor* funcs, they're all pretty much identical (so doesn't matter which one we call), but sucks that we can't determine this at runtime atm

    renderer::TitaniumPhysicalRenderingDevice renderingDevice {};
    LOG_CALL( renderer::InitialisePhysicalRenderingDevice( &renderingDevice ) );

    renderer::TitaniumRendererState rendererState {};
    LOG_CALL( renderer::Initialise( &renderingDevice, &rendererState, psdlWindow ) );

    #if USE_TESTS
    // TODO: change this when we have actual convars, and just read an init-only convar for this
    if ( util::commandline::HasArgument( &caCommandLine, "-dev:run_tests" ) ) 
    {
        LOG_CALL( dev::tests::RunTests() );
    }
    #endif // #if USE_TESTS

    bool bRunEngine = true;
    while ( bRunEngine )
    {
        {
            SDL_Event sdlEvent;
            while ( SDL_PollEvent( &sdlEvent ) )
            {
                ImGui_ImplSDL2_ProcessEvent( &sdlEvent );
                
                switch ( sdlEvent.type )
                {
                    case SDL_WINDOWEVENT:
                    {
                        switch ( sdlEvent.window.event )
                        {
                            case SDL_WINDOWEVENT_SIZE_CHANGED:
                            {
                                renderer::preframe::ResolutionChanged( &renderingDevice, &rendererState );
                                break;
                            }
                        }
    
                        break;
                    }
    
                    case SDL_QUIT:
                    {
                        bRunEngine = false;
                        break;
                    }
                }
            }
        }

        renderer::preframe::ImGUI( &rendererState ); 
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        //ImGui::ShowDemoWindow();

        renderer::Frame( &rendererState );
    }

    util::commandline::Free( &caCommandLine );
}
