#include "extern/imgui/imgui.h"
#include "extern/imgui/imgui_impl_sdl2.h"
#include "extern/imgui/imgui_impl_wgpu.h"

#include <SDL_gamecontroller.h>
#include <SDL_joystick.h>
#include <SDL_keycode.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>
#include <stdlib.h>

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_mouse.h>
#include <libtitanium/util/string.hpp>
#include <libtitanium/util/static_array.hpp>
#include <libtitanium/util/data/span.hpp>
#include <libtitanium/util/commandline.hpp>
#include <libtitanium/memory/mem_core.hpp>

// systems that need initialising
#include <libtitanium/sys/platform_sdl.hpp>
#include <libtitanium/config/config.hpp>
#include <libtitanium/filesystem/filesystem.hpp>
#include <libtitanium/jobsystem/jobsystem.hpp>
#include <libtitanium/logger/logger.hpp>
#include <libtitanium/renderer/renderer.hpp>
#include <libtitanium/imgui_widgets/widgets.hpp>

#include <libtitanium/input/input_device.hpp>
#include <libtitanium/input/input_actions.hpp>

#include <libtitanium/dev/tests.hpp>

#include "game_consolecommand.hpp"
#include "game_loadassimp.hpp"
#include "memory/mem_external.hpp"
#include "util/assert.hpp"
#include "util/maths.hpp"

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

static bool s_bRunGameLoop = true; config::Var * pcvarRunGameLoop = config::RegisterVar( "game::runloop", config::EFVarUsageFlags::STARTUP, config::VARF_BOOL, &s_bRunGameLoop );

static bool s_bCaptureMouse = false; config::Var * pcvarCaptureMouse = config::RegisterVar( "game::capturemouse", config::EFVarUsageFlags::NONE, config::VARF_BOOL, &s_bCaptureMouse );
static f32 s_flCameraFov = 20.f; config::Var * pcvarCameraFov = config::RegisterVar( "game::camerafov", config::EFVarUsageFlags::NONE, config::VARF_FLOAT, &s_flCameraFov );

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
        else
        {

        }
    }

    util::commandline::Free( &caCommandLine );

    char szProgramValue[32];
    pcvarProgram->setFuncs.fnToString( &s_eProgram, util::StaticArray_ToSpan( szProgramValue ) );

    logger::Info( "Hello world!" ENDL );
    logger::Info( "program = %s" ENDL, szProgramValue );

    if ( s_eProgram == EProgram::TESTS )
    {
        return dev::tests::RunTests() ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    //filesystem::Initialise();
    //jobsystem::Initialise();

    //memory::SetExternMemoryFunctions_SDL();
    // default to wayland where available
    SDL_SetHint( SDL_HINT_VIDEODRIVER, "wayland,x11" );
    SDL_Init( SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER );
    SDL_Window *const psdlWindow = SDL_CreateWindow( "Titanium - SDL + WGPU", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1600, 900, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN ); // TODO: do we need SDL_WINDOW_VULKAN/whatever api?

    // initialise imgui
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForVulkan( psdlWindow ); // TODO: looking at the ImGui_ImplSDL2_InitFor* funcs, they're all pretty much identical (so doesn't matter which one we call), but sucks that we can't determine this at runtime atm

    // init style
    ImGuiStyle *const pImguiStyle = &ImGui::GetStyle();
    [ pImguiStyle ](){
        // classic source vgui-like style
        // TODO: should be configurable in config files
        pImguiStyle->Colors[ ImGuiCol_Text ]                 = ImVec4( 0.81f, 0.81f, 0.81f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_TextDisabled ]         = ImVec4( 0.56f, 0.56f, 0.56f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_TextSelectedBg ]       = ImVec4( 0.12f, 0.37f, 0.75f, 0.50f );
        pImguiStyle->Colors[ ImGuiCol_WindowBg ]             = ImVec4( 0.27f, 0.27f, 0.27f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_ChildBg ]              = ImVec4( 0.00f, 0.00f, 0.00f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_PopupBg ]              = ImVec4( 0.27f, 0.27f, 0.27f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_Border ]               = ImVec4( 0.41f, 0.41f, 0.41f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_BorderShadow ]         = ImVec4( 0.04f, 0.04f, 0.04f, 0.64f );
        pImguiStyle->Colors[ ImGuiCol_FrameBg ]              = ImVec4( 0.13f, 0.13f, 0.13f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_FrameBgHovered ]       = ImVec4( 0.19f, 0.19f, 0.19f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_FrameBgActive ]        = ImVec4( 0.24f, 0.24f, 0.24f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_TitleBg ]              = ImVec4( 0.22f, 0.22f, 0.22f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_TitleBgActive ]        = ImVec4( 0.27f, 0.27f, 0.27f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_TitleBgCollapsed ]     = ImVec4( 0.00f, 0.00f, 0.00f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_MenuBarBg ]            = ImVec4( 0.22f, 0.22f, 0.22f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_ScrollbarBg ]          = ImVec4( 0.10f, 0.10f, 0.10f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_ScrollbarGrab ]        = ImVec4( 0.41f, 0.41f, 0.41f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_ScrollbarGrabHovered ] = ImVec4( 0.53f, 0.53f, 0.53f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_ScrollbarGrabActive ]  = ImVec4( 0.63f, 0.63f, 0.63f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_CheckMark ]            = ImVec4( 0.61f, 0.61f, 0.61f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_SliderGrab ]           = ImVec4( 0.41f, 0.41f, 0.41f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_SliderGrabActive ]     = ImVec4( 0.53f, 0.53f, 0.53f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_Button ]               = ImVec4( 0.35f, 0.35f, 0.35f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_ButtonHovered ]        = ImVec4( 0.45f, 0.45f, 0.45f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_ButtonActive ]         = ImVec4( 0.52f, 0.52f, 0.52f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_Header ]               = ImVec4( 0.35f, 0.35f, 0.35f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_HeaderHovered ]        = ImVec4( 0.45f, 0.45f, 0.45f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_HeaderActive ]         = ImVec4( 0.53f, 0.53f, 0.53f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_Separator ]            = ImVec4( 0.53f, 0.53f, 0.57f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_SeparatorHovered ]     = ImVec4( 0.53f, 0.53f, 0.53f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_SeparatorActive ]      = ImVec4( 0.63f, 0.63f, 0.63f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_ResizeGrip ]           = ImVec4( 0.41f, 0.41f, 0.41f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_ResizeGripHovered ]    = ImVec4( 0.52f, 0.52f, 0.52f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_ResizeGripActive ]     = ImVec4( 0.63f, 0.63f, 0.63f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_Tab ]                  = ImVec4( 0.18f, 0.18f, 0.18f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_TabHovered ]           = ImVec4( 0.39f, 0.39f, 0.39f, 1.00f );
        pImguiStyle->Colors[ ImGuiCol_TabActive ]            = ImVec4( 0.39f, 0.39f, 0.39f, 1.00f );

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
        return EXIT_FAILURE;
    }

    renderer::RenderView rendererMainView {
        .m_vCameraPosition { .x = -5.f, .y = -5.f, .z = 5.f },
        .m_vCameraRotation { .x = 320.f, .y = -130.f },
        .m_flCameraFOV = s_flCameraFov,
        .m_vRenderResolution = sys::sdl::GetWindowSizeVector( psdlWindow )
    };
    renderer::RenderView_Create( &rendererState, &rendererMainView );

    renderer::GPUModelHandle hHelmetModel;
    renderer::GPUTextureHandle gpuHelmetTextures[1];
    if ( !Assimp_LoadScene( &rendererState, "test_resource/damaged/DamagedHelmet.gltf", &hHelmetModel, util::StaticArray_ToSpan( gpuHelmetTextures ) ) )
    {
        logger::Info( "Required model load failed, exiting :c" ENDL );
        return EXIT_FAILURE;
    }

    renderer::RenderObject renderobjHelmet {
        .m_vPosition {},
        .m_vRotation { },
        .m_gpuModel = hHelmetModel,
        .m_gpuTexture = gpuHelmetTextures[ 0 ]
    };
    renderer::RenderObject_Create( &rendererState, &renderobjHelmet );
    renderer::RenderObject renderObjects[] {
        renderobjHelmet,
    };

    bool bShowConsole = false;
    char szConsoleInput[ 256 ] {};


    input::SetupSDL();

    // i would be very surprised if someone plugs in more than 16 controllers
    input::InputDevice inputDevices[16] {};
    input::InputDevice_InitialiseKeyboard( inputDevices ); // default the first input device to keyboard and mouse

    // make our actual binding objects
    // TODO: make user-configurable, through config var
    input::AnalogueBinding analogueBinds[ util::StaticArray_Length( s_AnalogueBindDefinitions ) ];
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
    float flAnalogueInputActionValues[ util::StaticArray_Length( s_AnalogueBindDefinitions ) ];

    input::DigitalBinding digitalBinds[ util::StaticArray_Length( s_DigitalBindDefinitions ) ];
    for ( uint i = 0; i < util::StaticArray_Length( s_DigitalBindDefinitions ); i++ )
    {
        digitalBinds[ i ] = {
            .eKBButton = s_DigitalBindDefinitions[ i ].eDefaultKBButton,
            .eKBAxis = s_DigitalBindDefinitions[ i ].eDefaultKBAxis,
            .eControllerButton = s_DigitalBindDefinitions[ i ].eDefaultControllerButton,
            .eControllerAxis = s_DigitalBindDefinitions[ i ].eDefaultControllerAxis
        };
    }
    u8 nDigitalInputActionValues[ input::SizeNeededForDigitalActions( util::StaticArray_Length( s_DigitalBindDefinitions ) ) ];

    SDL_SetWindowGrab( psdlWindow, SDL_TRUE );

    u64 nTimeNow = SDL_GetPerformanceCounter();
    u64 nTimeLastFrame;

    bool bRunGame = s_bRunGameLoop;
    while ( bRunGame )
    {
        nTimeLastFrame = nTimeNow;
        nTimeNow = SDL_GetPerformanceCounter();

        // TODO: temp, reexamine
        float flDeltaTime = (double)((nTimeNow - nTimeLastFrame)*1000 / (double)SDL_GetPerformanceFrequency()) / 1000;

        {
            SDL_Event sdlEvent;
            while ( SDL_PollEvent( &sdlEvent ) )
            {
                ImGui_ImplSDL2_ProcessEvent( &sdlEvent );
                input::ProcessSDLInputEvent( &sdlEvent, util::StaticArray_ToSpan( inputDevices ) );

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

        input::ProcessAnalogueActions( util::StaticArray_ToSpan( inputDevices ), util::StaticArray_ToSpan( analogueBinds ), util::StaticArray_ToSpan( flAnalogueInputActionValues ), flDeltaTime );
        input::ProcessDigitalActions( util::StaticArray_ToSpan( inputDevices ), util::StaticArray_ToSpan( digitalBinds ), util::StaticArray_ToSpan( nDigitalInputActionValues ) );

        float flXMove = input::AnalogueActionValue( util::StaticArray_ToSpan( flAnalogueInputActionValues ), EAnalogueInputActions::LOOKRIGHT );
        float flYMove = input::AnalogueActionValue( util::StaticArray_ToSpan( flAnalogueInputActionValues ), EAnalogueInputActions::LOOKUP );
        if ( input::DigitalActionHeld( util::StaticArray_ToSpan( nDigitalInputActionValues ), EDigitalInputActions::GRAB_OBJECT ) )
        {
            renderObjects[ 0 ].m_vRotation.x += flXMove;
            renderObjects[ 0 ].m_vRotation.y += flYMove;
            renderObjects[ 0 ].m_bGPUDirty = true; // we've changed the state of the object, we need to tell the renderer to write the new data to the gpu
        }
        else
        {
            rendererMainView.m_vCameraRotation.x += flXMove;
            rendererMainView.m_vCameraRotation.y += flYMove;
        }

        rendererMainView.m_flCameraFOV = s_flCameraFov += input::AnalogueActionValue( util::StaticArray_ToSpan( flAnalogueInputActionValues ), EAnalogueInputActions::ZOOM );
        rendererMainView.m_bGPUDirty = true;

        input::PostProcess( util::StaticArray_ToSpan( inputDevices ) );

        if ( input::DigitalActionPressed( util::StaticArray_ToSpan( nDigitalInputActionValues ), EDigitalInputActions::TOGGLECONSOLE ) )
        {
            bShowConsole = !bShowConsole;
            SDL_SetRelativeMouseMode( bShowConsole ? SDL_FALSE : SDL_TRUE );
        }

        if ( bShowConsole )
        {
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
            ImGui::Text("%f", flDeltaTime);

            ImGui::Text( "Camera: %fdeg { %f %f %f } { %f %f %f }", rendererMainView.m_flCameraFOV,
                                                                   rendererMainView.m_vCameraPosition.x, rendererMainView.m_vCameraPosition.y, rendererMainView.m_vCameraPosition.z,
                                                                   rendererMainView.m_vCameraRotation.x, rendererMainView.m_vCameraRotation.y, rendererMainView.m_vCameraRotation.z );

            ImGui::Text( "Rendering %i objects:", util::StaticArray_Length( renderObjects ) );
            for ( uint i = 0; i < util::StaticArray_Length( renderObjects ); i++ )
            {
                ImGui::Text( "\tObject %i: { %f %f %f } { %f %f %f }", i, renderObjects[ i ].m_vPosition.x, renderObjects[ i ].m_vPosition.y, renderObjects[ i ].m_vPosition.z,
                           renderObjects[ i ].m_vRotation.x, renderObjects[ i ].m_vRotation.y, renderObjects[ i ].m_vRotation.z );
            }

            ImGui::End();
        }

        renderer::Frame( &rendererState, &rendererMainView, util::StaticArray_ToSpan( renderObjects ) );
    }

    logger::Info( "Game is over. Exiting..." ENDL );

    // free all loaded models
    for ( uint i = 0; i <  util::StaticArray_Length( renderObjects ); i++ )
    {
        // TODO: causes malloc assert seemingly?
        renderer::FreeGPUModel( renderObjects[ i ].m_gpuModel );
        renderer::RenderObject_Free( &renderObjects[ i ] );
    }

    config::FreeVars(); // TODO: this sucks

    ImGui::DestroyContext();

    SDL_DestroyWindow( psdlWindow );
    SDL_Quit();

    logger::Info( "%i unfreed allocations" ENDL, memory::GetAllocs() );

    return EXIT_SUCCESS;
}
