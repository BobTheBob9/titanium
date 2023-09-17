#include "extern/imgui/imgui.h"
#include "extern/imgui/imgui_impl_sdl2.h"
#include "extern/imgui/imgui_impl_wgpu.h"

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <libtitanium/util/commandline.hpp>
#include <libtitanium/util/string.hpp>
#include <libtitanium/util/data/staticspan.hpp>
#include <libtitanium/memory/mem_core.hpp>

// systems that need initialising
#include <libtitanium/sys/platform_sdl.hpp>
#include <libtitanium/config/config.hpp>
#include <libtitanium/filesystem/filesystem.hpp>
#include <libtitanium/jobsystem/jobsystem.hpp>
#include <libtitanium/logger/logger.hpp>
#include <libtitanium/renderer/renderer.hpp>
#include <libtitanium/imgui_widgets/widgets.hpp>

#include <libtitanium/dev/tests.hpp>

#include "game_consolecommand.hpp"
#include "game_loadassimp.hpp"

static bool s_bRunTests = false; config::Var cvarRunTests = config::RegisterVar( "dev::runtests", config::EFVarUsageFlags::STARTUP, config::VARFUNCS_BOOL, &s_bRunTests );
static bool s_bExitAfterTests = true; config::Var cvarExitAfterTests = config::RegisterVar( "dev::exitaftertests", config::EFVarUsageFlags::STARTUP, config::VARFUNCS_BOOL, &s_bExitAfterTests );

static bool s_bRunGameLoop = true; config::Var cvarRunGameLoop = config::RegisterVar( "game::runloop", config::EFVarUsageFlags::STARTUP, config::VARFUNCS_BOOL, &s_bRunGameLoop );
static bool s_bCaptureMouse = false; config::Var cvarCaptureMouse = config::RegisterVar( "game::capturemouse", config::EFVarUsageFlags::STARTUP, config::VARFUNCS_BOOL, &s_bCaptureMouse );

static bool s_bShowImguiDemo = false; config::Var cvarShowImguiDemo = config::RegisterVar( "dev::imguidemo", config::EFVarUsageFlags::NONE, config::VARFUNCS_BOOL, &s_bShowImguiDemo );

int main( const int nArgs, const char *const *const ppszArgs )
{
    logger::Info( "Hello world!" ENDL );

    // grab commandline args
    util::commandline::CommandArgs caCommandLine {};
    util::commandline::CreateFromSystemWithAlloc( &caCommandLine, nArgs, ppszArgs );

    // set config vars from commandline, everything preceded with + is treated as a var
    // TODO: somehow, convar system needs to be able to handle setting a var before it exists, and resetting it after death (e.g. if a weapon registers a var, then unregisters, then reregisters, keep the user's value across all that)
    util::commandline::R_FindArgumentPair argIterator {};
    while ( ( argIterator = util::commandline::GetNextArgumentPairByWildcard( &caCommandLine, "+", 1, argIterator.pszValue ) ).bFound )
    {
        config::Var *const pVar = config::FindVar( argIterator.pszKey + 1 );
        if ( pVar )
        {
            pVar->setFuncs.fnSetFromString( pVar->pValue, argIterator.pszValue );
        }
    }

    #if USE_TESTS
    if ( s_bRunTests )
    {
        logger::Info( "dev::runtests == true, running tests..." ENDL );

        bool bTestResult = dev::tests::RunTests();

        if ( s_bExitAfterTests )
        {
            logger::Info( "tests are complete and dev::exitaftertests == true, exiting!" ENDL );
            return !bTestResult;
        }
    }
    #endif // #if USE_TESTS

    //filesystem::Initialise();
    //jobsystem::Initialise();

    // default to wayland where available
    SDL_SetHint( SDL_HINT_VIDEODRIVER, "wayland,x11" );

    SDL_Init( SDL_INIT_VIDEO );
    SDL_Window *const psdlWindow = SDL_CreateWindow( "Titanium - SDL + WGPU", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1600, 900, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN ); // TODO: do we need SDL_WINDOW_VULKAN/whatever api?

    // initialise imgui
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForVulkan( psdlWindow ); // TODO: looking at the ImGui_ImplSDL2_InitFor* funcs, they're all pretty much identical (so doesn't matter which one we call), but sucks that we can't determine this at runtime atm

    // init style
    ImGuiStyle *const pImguiStyle = &ImGui::GetStyle();
    [ pImguiStyle ](){
        // classic source vgui-like style
        // TODO: should be configurable in config files
        pImguiStyle->Colors[ImGuiCol_Text]                   = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_TextDisabled]           = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.12f, 0.37f, 0.75f, 0.50f);
        pImguiStyle->Colors[ImGuiCol_WindowBg]               = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_PopupBg]                = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_Border]                 = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_BorderShadow]           = ImVec4(0.04f, 0.04f, 0.04f, 0.64f);
        pImguiStyle->Colors[ImGuiCol_FrameBg]                = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_FrameBgActive]          = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_TitleBg]                = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_TitleBgActive]          = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_MenuBarBg]              = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_CheckMark]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_SliderGrab]             = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_Button]                 = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_ButtonHovered]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_ButtonActive]           = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_Header]                 = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_Separator]              = ImVec4(0.53f, 0.53f, 0.57f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_SeparatorActive]        = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_ResizeGrip]             = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_TabHovered]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        pImguiStyle->Colors[ImGuiCol_TabActive]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);

        pImguiStyle->WindowBorderSize  = 0.0f;
        pImguiStyle->FrameBorderSize   = 1.0f;
        pImguiStyle->ChildBorderSize   = 1.0f;
        pImguiStyle->PopupBorderSize   = 1.0f;
        pImguiStyle->TabBorderSize     = 1.0f;

        pImguiStyle->WindowRounding    = 4.0f;
        pImguiStyle->FrameRounding     = 1.0f;
        pImguiStyle->ChildRounding     = 1.0f;
        pImguiStyle->PopupRounding     = 3.0f;
        pImguiStyle->TabRounding       = 1.0f;
        pImguiStyle->ScrollbarRounding = 1.0f;
    }();

    renderer::TitaniumPhysicalRenderingDevice renderingDevice {};
    renderer::InitialisePhysicalRenderingDevice( &renderingDevice );

    renderer::TitaniumRendererState rendererState {};
    if ( !renderer::Initialise( &renderingDevice, &rendererState, psdlWindow ) )
    {
        return 1;
    }

    renderer::RenderView rendererMainView {
        .m_vCameraPosition { .x = 0.f, .y = 0.f, .z = 5.f },
        .m_vRenderResolution = sys::sdl::GetWindowSizeVector( psdlWindow )
    };
    renderer::RenderView_Create( &rendererState, &rendererMainView );

    renderer::GPUModelHandle hHelmetModel;
    util::data::StaticSpan<renderer::GPUTextureHandle, 1> sgpuHelmetTextures;
    if ( !Assimp_LoadScene( &rendererState, "test_resource/damaged/DamagedHelmet.gltf", &hHelmetModel, sgpuHelmetTextures.ToSpan() ) )
    {
        logger::Info( "Required model load failed, exiting :c" ENDL );
        return 1;
    }

    renderer::RenderObject renderobjHelmet {
        .m_vPosition {},
        .m_vRotation { },
        .m_gpuModel = hHelmetModel,
        .m_gpuTexture = sgpuHelmetTextures.m_tData[ 0 ]
    };
    renderer::RenderObject_Create( &rendererState, &renderobjHelmet );

    util::data::StaticSpan<renderer::RenderObject, 1> sRenderObjects {
        renderobjHelmet,
    };

    constexpr int CONSOLE_INPUT_SIZE = 256;
    util::data::StaticSpan<char, CONSOLE_INPUT_SIZE> spszConsoleInput {};

    if ( s_bCaptureMouse )
    {
        SDL_SetWindowMouseGrab( psdlWindow, SDL_TRUE );
        SDL_SetRelativeMouseMode( SDL_TRUE );
    }

    bool bRunGame = s_bRunGameLoop;
    while ( bRunGame )
    {
        util::maths::Vec2<i32> vMouseMove {};

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
                                renderer::ResolutionChanged( &renderingDevice, &rendererState, &rendererMainView, sys::sdl::GetWindowSizeVector( psdlWindow ) );
                                break;
                            }
                        }
    
                        break;
                    }

                    /*case SDL_MOUSEMOTION:
                    {
                        util::maths::Vec2<u32> vWindowSize = sys::sdl::GetWindowSizeVector( psdlWindow );
                        SDL_WarpMouseInWindow( psdlWindow, vWindowSize.x / 2, vWindowSize.y / 2 );

                        vMouseMove = { .x = sdlEvent.motion.x - (int)( vWindowSize.x / 2 ), .y = sdlEvent.motion.y - (int)( vWindowSize.y / 2 ) };

                        break;
                    }*/

                    case SDL_QUIT:
                    {
                        bRunGame = false;
                        break;
                    }
                }
            }
        }

        renderer::Preframe_ImGUI( &rendererState );
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        imguiwidgets::Console( spszConsoleInput.ToSpan(), nullptr, C_ConsoleAutocomplete, C_ConsoleCommandCompletion );

        if ( s_bShowImguiDemo )
        {
            ImGui::ShowDemoWindow( &s_bShowImguiDemo );
        }

        if ( ImGui::IsKeyDown( ImGuiKey_UpArrow ) )
        {
            vMouseMove.y -= 10;
        }

        if ( ImGui::IsKeyDown( ImGuiKey_DownArrow ) )
        {
            vMouseMove.y += 10;
        }

        if ( ImGui::IsKeyDown( ImGuiKey_RightArrow ) )
        {
            vMouseMove.x += 10;
        }

        if ( ImGui::IsKeyDown( ImGuiKey_LeftArrow ) )
        {
            vMouseMove.x -= 10;
        }

        float vfMouseMoveX = vMouseMove.x;
        float vfMouseMoveY = vMouseMove.y;

        rendererMainView.m_vCameraRotation.x = fmod( rendererMainView.m_vCameraRotation.x + vfMouseMoveX * .1f, 360.f );
        rendererMainView.m_vCameraRotation.y = fmod( rendererMainView.m_vCameraRotation.y + vfMouseMoveY * .1f, 360.f );


        if ( ImGui::IsKeyDown( ImGuiKey_W ) )
        {
            rendererMainView.m_vCameraPosition.z += 2.f;
            rendererMainView.m_bGPUDirty = true;
        }

        if ( ImGui::IsKeyDown( ImGuiKey_S ) )
        {
            rendererMainView.m_vCameraPosition.z -= 2.f;
            rendererMainView.m_bGPUDirty = true;
        }

        //rendererMainView.m_vCameraRotation.z = fmod( rendererState.m_nFramesRendered / 50.f, 360 );
        if ( vMouseMove.x != 0 || vMouseMove.y != 0 )
        {
            rendererMainView.m_bGPUDirty = true;
        }

        sRenderObjects.m_tData[ 0 ].m_vRotation.x = fmod( rendererState.m_nFramesRendered / 50.f, 360 );
        sRenderObjects.m_tData[ 0 ].m_bGPUDirty = true; // we've changed the state of the object, we need to tell the renderer to write the new data to the gpu

        if ( imguiwidgets::BeginDebugOverlay() )
        {
            ImGui::Text( "Camera: { %f %f %f } { %f %f %f }", rendererMainView.m_vCameraPosition.x, rendererMainView.m_vCameraPosition.y, rendererMainView.m_vCameraPosition.z,
                                                                   rendererMainView.m_vCameraRotation.x, rendererMainView.m_vCameraRotation.y, rendererMainView.m_vCameraRotation.z );

            ImGui::Text( "Rendering %i objects:", (int)sRenderObjects.Elements() );
            for ( int i = 0; i < sRenderObjects.Elements(); i++ )
            {
                ImGui::Text( "\tObject %i: { %f %f %f } { %f %f %f }", i, sRenderObjects.m_tData[ i ].m_vPosition.x, sRenderObjects.m_tData[ i ].m_vPosition.y, sRenderObjects.m_tData[ i ].m_vPosition.z,
                           sRenderObjects.m_tData[ i ].m_vRotation.x, sRenderObjects.m_tData[ i ].m_vRotation.y, sRenderObjects.m_tData[ i ].m_vRotation.z );
            }

            ImGui::End();
        }

        renderer::Frame( &rendererState, &rendererMainView, util::data::Span<renderer::RenderObject>( sRenderObjects.Elements(), sRenderObjects.m_tData ) );
    }

    // free all loaded models
    for ( int i = 0; i < sRenderObjects.Elements(); i++ )
    {
        // TODO: causes malloc assert seemingly?
        //renderer::FreeGPUModel( sRenderObjects.m_tData[ i ].m_gpuModel );
        renderer::RenderObject_Free( &sRenderObjects.m_tData[ i ] );
    }

    config::StaticFree(); // TODO: this sucks

    util::commandline::Free( &caCommandLine );

    logger::Info( "%i unfreed allocations" ENDL, memory::GetAllocs() );

    return 0;
}
