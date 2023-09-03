#include "extern/imgui/imgui.h"
#include "extern/imgui/imgui_impl_sdl2.h"
#include "extern/imgui/imgui_impl_wgpu.h"

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

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "game_consolecommand.hpp"
#include "util/maths.hpp"

config::Var<bool> * g_pbcvarRunTests = config::RegisterVar<bool>( "dev:runtests", false, config::EFVarUsageFlags::STARTUP );
config::Var<bool> * g_pbcvarRunGame = config::RegisterVar<bool>( "game:startloop", true, config::EFVarUsageFlags::STARTUP );

config::Var<bool> * g_pbcvarShowImguiDemo = config::RegisterVar<bool>( "dev:imguidemo", false, config::EFVarUsageFlags::NONE );

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
        config::IVarAny *const pVarAny = config::FindVarUntyped( argIterator.pszKey + 1 );
        if ( pVarAny )
        {
            pVarAny->V_SetFromString( argIterator.pszValue );
        }
    }

    //filesystem::Initialise();
    //jobsystem::Initialise();

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
    renderer::Initialise( &renderingDevice, &rendererState, psdlWindow );

    renderer::RenderView rendererMainView { .m_vCameraPosition = util::maths::Vec3<f32>( 0.f, 0.f, 200.f ) };
    renderer::CreateRenderView( &rendererState, &rendererMainView, sys::sdl::GetWindowSizeVector( psdlWindow ) );

    #if USE_TESTS
    if ( g_pbcvarRunTests->tValue ) 
    {
        LOG_CALL( dev::tests::RunTests() );
    }
    #endif // #if USE_TESTS

    // TEMP: assimp model load
    auto fnLoadAssimpModel = [ &rendererState ]( const char *const pszModelName ){
        const aiScene *const passimpLoadedModel = aiImportFile( pszModelName, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_Triangulate );
        logger::Info( "%s" ENDL, passimpLoadedModel != nullptr ? "Loaded model! c:" : "Didn't load model :c (we will probably crash now)" );

        assert::Release( passimpLoadedModel->mNumMeshes == 1 );
        const aiMesh *const passimpLoadedMesh = passimpLoadedModel->mMeshes[ 0 ];

        logger::Info( "Model has %i vertices and %i faces" ENDL, passimpLoadedMesh->mNumVertices, passimpLoadedMesh->mNumFaces );

        ::util::data::Span<float> sflVertexes( passimpLoadedMesh->mNumVertices * 3, reinterpret_cast<float *>( passimpLoadedMesh->mVertices ) );
        ::util::data::Span<u16> snIndexes( passimpLoadedMesh->mNumFaces * 3, memory::alloc_nT<u16>( passimpLoadedMesh->mNumFaces * 3 ) );
        for ( int i = 0; i < passimpLoadedMesh->mNumFaces; i++ )
        {
            assert::Release( passimpLoadedMesh->mFaces[ i ].mNumIndices == 3 );

            snIndexes.m_pData[ i * 3 ] = passimpLoadedMesh->mFaces[ i ].mIndices[ 0 ];
            snIndexes.m_pData[ i * 3 + 1 ] = passimpLoadedMesh->mFaces[ i ].mIndices[ 1 ];
            snIndexes.m_pData[ i * 3 + 2 ] = passimpLoadedMesh->mFaces[ i ].mIndices[ 2 ];
        }

        return renderer::UploadModel( &rendererState, sflVertexes, snIndexes );
    };

    renderer::GPUModelHandle rangerGPUModel = fnLoadAssimpModel( "Quakeguy_Ranger_Rigged.obj" );
    renderer::RenderableObject rangerRenderable {
        .m_vPosition {},
        .m_vRotation {},
        .m_gpuModel = rangerGPUModel
    };
    CreateRenderableObjectBuffers( &rendererState, &rangerRenderable );

    renderer::GPUModelHandle boxGPUModel = fnLoadAssimpModel( "mp_box.bsp.gltf" );
    renderer::RenderableObject rangerRenderable2 {
        .m_vPosition {},
        .m_vRotation {},
        .m_gpuModel = boxGPUModel
    };
    CreateRenderableObjectBuffers( &rendererState, &rangerRenderable2 );

    util::data::StaticSpan<renderer::RenderableObject, 1> sRenderableObjects {
        rangerRenderable,
        rangerRenderable2
    };

    constexpr int CONSOLE_INPUT_SIZE = 256;
    util::data::Span<char> spszConsoleInput( CONSOLE_INPUT_SIZE, memory::alloc_nT<char>( CONSOLE_INPUT_SIZE ) );
    memset( spszConsoleInput.m_pData, 0, CONSOLE_INPUT_SIZE );

    bool bRunGame = g_pbcvarRunGame->tValue;
    while ( bRunGame )
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
                                renderer::preframe::ResolutionChanged( &renderingDevice, &rendererState, &rendererMainView, sys::sdl::GetWindowSizeVector( psdlWindow ) );
                                break;
                            }
                        }
    
                        break;
                    }
    
                    case SDL_QUIT:
                    {
                        bRunGame = false;
                        break;
                    }
                }
            }
        }

        renderer::preframe::ImGUI( &rendererState );
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        imguiwidgets::Console( spszConsoleInput, nullptr, C_ConsoleAutocomplete, C_ConsoleCommandCompletion );

        if ( g_pbcvarShowImguiDemo->tValue )
        {
            ImGui::ShowDemoWindow( &g_pbcvarShowImguiDemo->tValue );
        }

        sRenderableObjects.m_tData[ 0 ].m_vRotation.z = fmod( rendererState.m_nFramesRendered / 50.f, 360 );
        sRenderableObjects.m_tData[ 0 ].m_bGPUDirty = true; // we've changed the state of the object, we need to tell the renderer to write the new data to the gpu

        renderer::Frame( &rendererState, &rendererMainView, util::data::Span<renderer::RenderableObject>( sRenderableObjects.Elements(), sRenderableObjects.m_tData ) );
    }

    // free all loaded models
    for ( int i = 0; i < sRenderableObjects.Elements(); i++ )
    {
        renderer::FreeGPUModel( sRenderableObjects.m_tData[ i ].m_gpuModel );
    }

    memory::free( spszConsoleInput.m_pData );
    config::FreeVars(); // TODO: this sucks

    util::commandline::Free( &caCommandLine );

    logger::Info( "%i unfreed allocations" ENDL, memory::GetAllocs() );

    return 0;
}
