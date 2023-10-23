#include <SDL.h>
#include <SDL_events.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>
#include <libtitanium/sys/platform_sdl.hpp>

#include <stdlib.h>

#include <libtitanium/util/maths.hpp>
#include <libtitanium/util/string.hpp>
#include <libtitanium/util/assert.hpp>
#include <libtitanium/util/static_array.hpp>
#include <libtitanium/util/data/span.hpp>
#include <libtitanium/util/commandline.hpp>
#include <libtitanium/memory/mem_core.hpp>
#include <libtitanium/memory/mem_external.hpp>

// systems that need initialising
#include <libtitanium/config/config.hpp>
#include <libtitanium/filesystem/filesystem.hpp>
#include <libtitanium/jobsystem/jobsystem.hpp>
#include <libtitanium/logger/logger.hpp>
#include <libtitanium/renderer/renderer.hpp>
#include <libtitanium/imgui_widgets/widgets.hpp>

#include <libtitanium/input/input_device.hpp>
#include <libtitanium/input/input_actions.hpp>

#include <libtitanium/dev/tests.hpp>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_wgpu.h>

#include "game_consolecommand.hpp"
#include "game_loadassimp.hpp"

enum class EProgram
{
    GAME,

#if HAS_TESTS
    TESTS
#endif // #if HAS_TESTS
};

static EProgram s_eProgram = EProgram::GAME; config::Var * pcvarProgram = config::RegisterVar( "program", config::EFVarUsageFlags::STARTUP, {
    .fnToString = []( const void *const pCvarPointer, util::data::Span<char> o_spszOutputBuffer ) {
        const EProgram eProgram = *static_cast<const EProgram *const>( pCvarPointer );

        const char * pszStringValue = "";
        if ( eProgram == EProgram::GAME )
        {
            pszStringValue = "game";
        }
#if HAS_TESTS
        else if ( eProgram == EProgram::TESTS )
        {
            pszStringValue = "tests";
        }
#endif // #if HAS_TESTS

        util::string::CopyTo( pszStringValue, o_spszOutputBuffer );
    },

    .fnSuggestValues = []( const void *const pCvarPointer, const char *const pszIncompleteValue, util::data::Span<util::data::StringBuf<32>> o_sspszOutputBuffer )
    {
        (void)pCvarPointer; (void)pszIncompleteValue; (void)o_sspszOutputBuffer;
    },

    .fnSetFromString = []( void *const pCvarPointer, const char *const pszValue ){
        util::data::StringBuf<16> pszLowercaseValue = pszValue;
        util::string::ToLowercase( pszLowercaseValue.m_szStr );

        EProgram eSetProgram = EProgram::GAME;
#if HAS_TESTS
        if ( util::string::CStringsEqual( pszLowercaseValue, "tests" ) )
        {
            eSetProgram = EProgram::TESTS;
        }
#endif // #if HAS_TESTS

        *static_cast<EProgram* const>( pCvarPointer ) = eSetProgram;
    }
}, &s_eProgram );

static bool s_bImguiDockingEnable = true; config::Var * pcvarImguiDockingEnable = config::RegisterVar( "imgui::docking", config::EFVarUsageFlags::STARTUP, config::VARF_BOOL, &s_bImguiDockingEnable );
static bool s_bImguiMultiViewportEnable = true; config::Var * pcvarImguiMultiViewportEnable = config::RegisterVar( "imgui::multiviewport", config::EFVarUsageFlags::STARTUP, config::VARF_BOOL, &s_bImguiMultiViewportEnable );

static bool s_bRunGameLoop = true; config::Var * pcvarRunGameLoop = config::RegisterVar( "game::runloop", config::EFVarUsageFlags::STARTUP, config::VARF_BOOL, &s_bRunGameLoop );

static bool s_bCaptureMouse = false; config::Var * pcvarCaptureMouse = config::RegisterVar( "game::capturemouse", config::EFVarUsageFlags::NONE, config::VARF_BOOL, &s_bCaptureMouse );
static f32 s_flCameraFov = 120.f; config::Var * pcvarCameraFov = config::RegisterVar( "game::camerafov", config::EFVarUsageFlags::NONE, config::VARF_FLOAT, &s_flCameraFov );

struct AnalogueBindDefinition
{
    char szName[32];
    input::EKeyboardMouseButton eDefaultKBButtonPos;
    input::EKeyboardMouseButton eDefaultKBButtonNeg;
    input::EKeyboardMouseAxis eDefaultKBAxis;
    input::EControllerButton eDefaultControllerButtonPos;
    input::EControllerButton eDefaultControllerButtonNeg;
    input::EControllerAxis eDefaultControllerAxis;
};

namespace EAnalogueInputActions
{
    enum EAnalogueInputActions
    {
        LOOKRIGHT,
        LOOKUP,
        ZOOM,
        MOVERIGHT,
        MOVEUP,
    };
}

AnalogueBindDefinition s_AnalogueBindDefinitions[] {
    { // EAnalogueInputActions::LOOKRIGHT
        .szName = "lookright",
        .eDefaultKBButtonPos = input::EKeyboardMouseButton::KEYBOARD_RIGHTARROW,
        .eDefaultKBButtonNeg = input::EKeyboardMouseButton::KEYBOARD_LEFTARROW,
        .eDefaultKBAxis = input::EKeyboardMouseAxis::MOUSE_MOVE_X,
        .eDefaultControllerAxis = input::EControllerAxis::RIGHTSTICK_X
    },
    { // EAnalogueInputActions::LOOKUP
        .szName = "lookup",
        .eDefaultKBButtonPos = input::EKeyboardMouseButton::KEYBOARD_UPARROW,
        .eDefaultKBButtonNeg = input::EKeyboardMouseButton::KEYBOARD_DOWNARROW,
        .eDefaultKBAxis = input::EKeyboardMouseAxis::MOUSE_MOVE_Y,
        .eDefaultControllerAxis = input::EControllerAxis::RIGHTSTICK_Y
    },
    { // EAnalogueInputActions::ZOOM
        .szName = "zoom",
        .eDefaultKBAxis = input::EKeyboardMouseAxis::MOUSE_WHEEL,
        .eDefaultControllerButtonPos = input::EControllerButton::DPAD_DOWN,
        .eDefaultControllerButtonNeg = input::EControllerButton::DPAD_UP
    },
    { // EAnalogueInputActions::MOVERIGHT
        .szName = "moveright",
        .eDefaultControllerButtonPos = input::EControllerButton::X,
        .eDefaultControllerButtonNeg = input::EControllerButton::B,
        .eDefaultControllerAxis = input::EControllerAxis::LEFTSTICK_X
    },
    { // EAnalogueInputActions::MOVEUP
        .szName = "moveup",
        .eDefaultControllerButtonPos = input::EControllerButton::Y,
        .eDefaultControllerButtonNeg = input::EControllerButton::A,
        .eDefaultControllerAxis = input::EControllerAxis::LEFTSTICK_Y
    },
};

struct DigitalBindDefinition
{
    char szName[32];
    input::EKeyboardMouseButton eDefaultKBButton;
    input::EKeyboardMouseAxis eDefaultKBAxis;
    input::EControllerButton eDefaultControllerButton;
    input::EControllerAxis eDefaultControllerAxis;
};

namespace EDigitalInputActions
{
    enum EDigitalInputActions
    {
        GRAB_OBJECT,
        TOGGLECONSOLE
    };
}

DigitalBindDefinition s_DigitalBindDefinitions[] {
    { // EDigitalInputActions::GRAB_OBJECT
        .szName = "grab_object",
        .eDefaultKBButton = input::EKeyboardMouseButton::MOUSE_LEFT,
        //.eDefaultControllerAxis = input::EControllerAxis::TRIGGER_RIGHT
    },
    { // EDigitalInputActions::TOGGLECONSOLE
        .szName = "toggleconsole",
        .eDefaultKBButton = input::EKeyboardMouseButton::KEYBOARD_GRAVE,
        .eDefaultControllerButton = input::EControllerButton::RIGHTBUMPER,
    }
};

int main( const int nArgs, const char *const *const ppszArgs )
{
    // grab commandline args
    util::CommandArgs caCommandLine {};
    util::CommandArgs::CreateFromSystemWithAlloc( &caCommandLine, nArgs, ppszArgs );

    // set config vars from commandline, everything preceded with + is treated as a var
    // TODO: somehow, convar system needs to be able to handle setting a var before it exists, and resetting it after death (e.g. if a weapon registers a var, then unregisters, then reregisters, keep the user's value across all that)
    util::CommandArgs::R_FindArgumentPair argIterator {};
    while ( ( argIterator = util::CommandArgs::GetNextArgumentPairByWildcard( &caCommandLine, "+", 1, argIterator.pszValue ) ).bFound )
    {
        config::Var *const pVar = config::FindVar( argIterator.pszKey + 1 );
        if ( pVar )
        {
            pVar->setFuncs.fnSetFromString( pVar->pValue, argIterator.pszValue );
        }
        else
        {
            // TODO: this
        }
    }

    util::CommandArgs::FreeMembers( &caCommandLine );

    char szProgramValue[32];
    pcvarProgram->setFuncs.fnToString( &s_eProgram, util::StaticArray_ToSpan( szProgramValue ) );

    logger::Info( "Hello world!" ENDL );
    logger::Info( "program = %s" ENDL, szProgramValue );

#if HAS_TESTS
    if ( s_eProgram == EProgram::TESTS )
    {
        return dev::tests::RunTests() ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    // ensure all memory committed for tests is cleaned up
    dev::tests::CleanupTests();
#endif // #if HAS_TESTS

    //filesystem::Initialise();
    //jobsystem::Initialise();

    memory::SetExternMemoryFunctions_SDL();
    // default to wayland where available
    SDL_SetHint( SDL_HINT_VIDEODRIVER, "wayland,x11" );
    SDL_Init( SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER );
    SDL_Window *const psdlWindow = SDL_CreateWindow( "Titanium - SDL + WGPU", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1600, 900, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN ); // TODO: do we need SDL_WINDOW_VULKAN/whatever api?

    // initialise imgui
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForVulkan( psdlWindow ); // TODO: looking at the ImGui_ImplSDL2_InitFor* funcs, they're all pretty much identical (so doesn't matter which one we call), but sucks that we can't determine this at runtime atm

    // TODO: multiple viewports are largely busted on linux, especially wayland :c
    // setting the flags doesn't seem problematic though
    if ( s_bImguiDockingEnable )
    {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    if ( s_bImguiMultiViewportEnable )
    {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    renderer::TitaniumPhysicalRenderingDevice renderingDevice {};
    renderer::InitialisePhysicalRenderingDevice( &renderingDevice );
    renderer::TitaniumRendererState rendererState {};
    if ( !renderer::Initialise( &renderingDevice, &rendererState, psdlWindow ) )
    {
        return EXIT_FAILURE;
    }

    renderer::RenderView rendererMainView {
        //.m_vCameraPosition = { .x = 20.f, .y = 20.f, .z = -20.f },
        .m_vCameraRotation = { .yaw = 180.f },
        .m_flCameraFOV = s_flCameraFov,
        .m_vRenderResolution = sys::sdl::GetWindowSizeVector( psdlWindow )
    };
    renderer::RenderView::Create( &rendererState, &rendererMainView );

    renderer::GPUModelHandle hHelmetModel;
    renderer::GPUTextureHandle gpuHelmetTextures[1];
    if ( !Assimp_LoadScene( &rendererState, "test_resource/damaged/DamagedHelmet.gltf", &hHelmetModel, util::StaticArray_ToSpan( gpuHelmetTextures ) ) )
    {
        logger::Info( "Required model load failed, exiting :c" ENDL );
        return EXIT_FAILURE;
    }

    renderer::RenderObject renderObjects[ ( 3 * 3 * 3 ) + 2 ];

    for ( int x = 0; x < 3; x++ )
    {
        for ( int y = 0; y < 3; y++ )
        {
            for ( int z = 0; z < 3; z++ )
            {
                const uint nIdx = z * 3 * 3 + y * 3 + x;
                renderObjects[ nIdx ] = {
                    .m_vPosition = { .x = ( x - 1 ) * 10.f, .y = ( y - 1 ) * 10.f, .z = ( z - 1 ) * 10.f },
                    .m_gpuModel = hHelmetModel,
                    .m_gpuTexture = gpuHelmetTextures[ 0 ]
                };

                renderer::RenderObject::Create( &rendererState, &renderObjects[ nIdx ] );
            }
        }
    }

    renderer::GPUModelHandle hPlaneModel;
    if ( !Assimp_LoadScene( &rendererState, "test_resource/plane.obj", &hPlaneModel, { .nLength = 0 } ) )
    {
        logger::Info( "Required model load failed, exiting :c" ENDL );
        return EXIT_FAILURE;
    }

    // too lazy to actually like, make a double sided plane model
    // so we are spawning 2
    // :)
    const uint nPlaneIndexBegin = 3 * 3 * 3;
    renderer::RenderObject *const pPlaneBottom = &renderObjects[ nPlaneIndexBegin ];

    *pPlaneBottom = { .m_gpuModel = hPlaneModel, .m_gpuTexture = gpuHelmetTextures[ 0 ] };
    renderer::RenderObject::Create( &rendererState, pPlaneBottom );

    renderer::RenderObject *const pPlaneTop = &renderObjects[ nPlaneIndexBegin + 1 ];
    *pPlaneTop = { .m_vRotation = { .pitch = 180.f }, .m_gpuModel = hPlaneModel, .m_gpuTexture = gpuHelmetTextures[ 0 ] };
    renderer::RenderObject::Create( &rendererState, pPlaneTop );

    bool bShowConsole = false;
    char szConsoleInput[ 256 ] {};


    input::SetupSDL();

    // i would be very surprised if someone plugs in more than 16 controllers
    input::InputDevice inputDevices[16] {};
    input::InputDevice_InitialiseKeyboard( inputDevices ); // default the first input device to keyboard and mouse

    // make our actual binding objects
    // TODO: make user-configurable, through config var
    input::AnalogueBinding analogueBinds[ util::StaticArray_Length( s_AnalogueBindDefinitions ) ];
    float flAnalogueInputActionValues[ util::StaticArray_Length( s_AnalogueBindDefinitions ) ];
    for ( uint i = 0; i < util::StaticArray_Length( s_AnalogueBindDefinitions ); i++ )
    {
        analogueBinds[ i ] = {
            .eKBButtonPos = s_AnalogueBindDefinitions[ i ].eDefaultKBButtonPos,
            .eKBButtonNeg = s_AnalogueBindDefinitions[ i ].eDefaultKBButtonNeg,
            .eKBAxis = s_AnalogueBindDefinitions[ i ].eDefaultKBAxis,
            .eControllerButtonPos = s_AnalogueBindDefinitions[ i ].eDefaultControllerButtonPos,
            .eControllerButtonNeg = s_AnalogueBindDefinitions[ i ].eDefaultControllerButtonNeg,
            .eControllerAxis = s_AnalogueBindDefinitions[ i ].eDefaultControllerAxis,
        };
    }

    input::DigitalBinding digitalBinds[ util::StaticArray_Length( s_DigitalBindDefinitions ) ];
    u8 nDigitalInputActionValues[ input::SizeNeededForDigitalActions( util::StaticArray_Length( s_DigitalBindDefinitions ) ) ];
    for ( uint i = 0; i < util::StaticArray_Length( s_DigitalBindDefinitions ); i++ )
    {
        digitalBinds[ i ] = {
            .eKBButton = s_DigitalBindDefinitions[ i ].eDefaultKBButton,
            .eKBAxis = s_DigitalBindDefinitions[ i ].eDefaultKBAxis,
            .eControllerButton = s_DigitalBindDefinitions[ i ].eDefaultControllerButton,
            .eControllerAxis = s_DigitalBindDefinitions[ i ].eDefaultControllerAxis
        };
    }

    SDL_SetWindowGrab( psdlWindow, SDL_TRUE );

    u64 nTimeNow = SDL_GetPerformanceCounter();
    u64 nTimeLastFrame;

    bool bRunGame = s_bRunGameLoop;
    while ( bRunGame )
    {
        nTimeLastFrame = nTimeNow;
        nTimeNow = SDL_GetPerformanceCounter();

        // TODO: temp, reexamine
        float flsecDeltaTime = (double)((nTimeNow - nTimeLastFrame)*1000 / (double)SDL_GetPerformanceFrequency()) / 1000;

        {
            SDL_Event sdlEvent;
            while ( SDL_PollEvent( &sdlEvent ) )
            {
                ImGui_ImplSDL2_ProcessEvent( &sdlEvent );

                if ( !ImGui::IsWindowFocused( ImGuiFocusedFlags_AnyWindow ) )
                {
                    input::ProcessSDLInputEvent( &sdlEvent, util::StaticArray_ToSpan( inputDevices ) );
                }

                switch ( sdlEvent.type )
                {
                    case SDL_WINDOWEVENT:
                    {
                        switch ( sdlEvent.window.event )
                        {
                            case SDL_WINDOWEVENT_SIZE_CHANGED:
                            {
                                const util::maths::Vec2<u32> vWindowSize = sys::sdl::GetWindowSizeVector( psdlWindow );
                                logger::Info( "Window resized to { %i %i }" ENDL, vWindowSize.x, vWindowSize.y );
                                renderer::ResolutionChanged( &renderingDevice, &rendererState, &rendererMainView, vWindowSize );
                                break;
                            }
                        }
    
                        break;
                    }

                    case SDL_QUIT:
                    {
                        logger::Info( "Recieved SDL_QUIT event" ENDL );
                        bRunGame = false;
                        break;
                    }
                }
            }
        }

        renderer::Preframe_ImGUI();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        input::ProcessAnalogueActions( util::StaticArray_ToSpan( inputDevices ), util::StaticArray_ToSpan( analogueBinds ), util::StaticArray_ToSpan( flAnalogueInputActionValues ), flsecDeltaTime );
        input::ProcessDigitalActions( util::StaticArray_ToSpan( inputDevices ), util::StaticArray_ToSpan( digitalBinds ), util::StaticArray_ToSpan( nDigitalInputActionValues ) );

        const util::maths::Vec3Angle<float> vflLookVector {
            .yaw = input::AnalogueActionValue( util::StaticArray_ToSpan( flAnalogueInputActionValues ), EAnalogueInputActions::LOOKRIGHT ),
            .pitch = input::AnalogueActionValue( util::StaticArray_ToSpan( flAnalogueInputActionValues ), EAnalogueInputActions::LOOKUP )
        };
        if ( input::DigitalActionHeld( util::StaticArray_ToSpan( nDigitalInputActionValues ), EDigitalInputActions::GRAB_OBJECT ) )
        {
            util::maths::Vec3Angle<float>::AddTo( &renderObjects[ 0 ].m_vRotation, vflLookVector );
            renderObjects[ 0 ].m_bGPUDirty = true; // we've changed the state of the object, we need to tell the renderer to write the new data to the gpu
        }
        else
        {
            util::maths::Vec3Angle<float>::AddTo( &rendererMainView.m_vCameraRotation, vflLookVector );
        }

        const float flForwardMove = input::AnalogueActionValue( util::StaticArray_ToSpan( flAnalogueInputActionValues ), EAnalogueInputActions::MOVEUP );
        const util::maths::Vec3<float> vfMoveVector = util::maths::Vec3<float>::MultiplyScalar( util::maths::AnglesToForward<float>( rendererMainView.m_vCameraRotation ), flForwardMove );
        util::maths::Vec3<float>::AddTo( &rendererMainView.m_vCameraPosition, vfMoveVector );

        rendererMainView.m_flCameraFOV = s_flCameraFov += input::AnalogueActionValue( util::StaticArray_ToSpan( flAnalogueInputActionValues ), EAnalogueInputActions::ZOOM );
        rendererMainView.m_bGPUDirty = true;

        input::PostProcess( util::StaticArray_ToSpan( inputDevices ) );

        if ( input::DigitalActionPressed( util::StaticArray_ToSpan( nDigitalInputActionValues ), EDigitalInputActions::TOGGLECONSOLE ) )
        {
            bShowConsole = !bShowConsole;
            SDL_SetRelativeMouseMode( bShowConsole ? SDL_FALSE : SDL_TRUE );
            SDL_SetWindowGrab( psdlWindow, bShowConsole ? SDL_FALSE : SDL_TRUE );
        }

        if ( bShowConsole )
        {
            ImGui::ShowStyleEditor();
            imguiwidgets::Console( util::StaticArray_ToSpan( szConsoleInput ), nullptr, C_ConsoleAutocomplete, C_ConsoleCommandCompletion );
        }

        if ( pcvarCameraFov->bDirty )
        {
            rendererMainView.m_flCameraFOV = s_flCameraFov;
            rendererMainView.m_bGPUDirty = true;

            pcvarCameraFov->bDirty = false;
        }

        if ( imguiwidgets::BeginDebugOverlay() )
        {
            ImGui::Text( "%.0f FPS (%fms)", 1 / flsecDeltaTime, flsecDeltaTime * 1000.f );
            //ImGui::Text( "Frame %i", flDeltaTime->m_nFramesRendered );

            ImGui::Text( "Camera: %fdeg { %f %f %f } { %f %f %f }", rendererMainView.m_flCameraFOV,
                                                                   rendererMainView.m_vCameraPosition.x, rendererMainView.m_vCameraPosition.y, rendererMainView.m_vCameraPosition.z,
                                                                   rendererMainView.m_vCameraRotation.yaw, rendererMainView.m_vCameraRotation.pitch, rendererMainView.m_vCameraRotation.roll );

            ImGui::Text( "Rendering %i objects:", util::StaticArray_Length( renderObjects ) );
            for ( uint i = 0; i < util::StaticArray_Length( renderObjects ); i++ )
            {
                ImGui::Text( "\tObject %i: { %f %f %f } { %f %f %f }", i, renderObjects[ i ].m_vPosition.x, renderObjects[ i ].m_vPosition.y, renderObjects[ i ].m_vPosition.z,
                           renderObjects[ i ].m_vRotation.yaw, renderObjects[ i ].m_vRotation.pitch, renderObjects[ i ].m_vRotation.roll );
            }

            ImGui::End();
        }

        renderer::Frame( &rendererState, &rendererMainView, util::StaticArray_ToSpan( renderObjects ) );
    }

    logger::Info( "Game is over. Exiting..." ENDL );

    // free loaded models and textures
    renderer::FreeGPUModel( hHelmetModel );
    for ( uint i = 0; i < util::StaticArray_Length( gpuHelmetTextures ); i++ )
    {
        renderer::FreeGPUTexture( gpuHelmetTextures[ i ] );
    }

    // free all renderer objects
    for ( uint i = 0; i <  util::StaticArray_Length( renderObjects ); i++ )
    {
        renderer::RenderObject::Free( &renderObjects[ i ] );
    }

    renderer::Shutdown( &rendererState );
    renderer::ShutdownDevice( &renderingDevice );

    config::FreeVars(); // TODO: this sucks

    ImGui::DestroyContext();

    SDL_DestroyWindow( psdlWindow );
    SDL_Quit();

#if HAS_MEM_DEBUG
    logger::Info( "%llu unfreed allocations" ENDL, memory::GetAllocs() );
    memory::ReportAllocationInfo();
#endif // #if HAS_MEM_DEBUG

    return EXIT_SUCCESS;
}
