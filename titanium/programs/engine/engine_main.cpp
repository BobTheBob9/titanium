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

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

int main( const int nArgs, const char *const *const ppszArgs )
{
    logger::Info( "Hello world! From Titanium!!!" ENDL );

    // grab commandline args
    util::commandline::CommandArgs caCommandLine {};
    util::commandline::CreateFromSystemWithAlloc( &caCommandLine, nArgs, ppszArgs );

    LOG_CALL( filesystem::Initialise() );
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

    // TEMP: assimp
    auto fnLoadAssimpModel = [ &rendererState ]( const char *const pszModelName ){
        const aiScene *const passimpLoadedModel = aiImportFile( pszModelName, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_Triangulate );
        logger::Info( "%s" ENDL, passimpLoadedModel != nullptr ? "Loaded model! c:" : "Didn't load model :c (we will probably crash now)" );

        assert::Release( passimpLoadedModel->mNumMeshes == 1 );
        const aiMesh *const passimpLoadedMesh = passimpLoadedModel->mMeshes[ 0 ];

        logger::Info( "Model has %i vertices and %i faces" ENDL, passimpLoadedMesh->mNumVertices, passimpLoadedMesh->mNumFaces );

        ::util::data::Span<float> sflVertexes( passimpLoadedMesh->mNumVertices * 3, memory::alloc_nT<float>( passimpLoadedMesh->mNumVertices * 3 ) );
        for ( int i = 0; i < passimpLoadedMesh->mNumVertices; i++ )
        {
            sflVertexes.m_pData[ i * 3 ] = passimpLoadedMesh->mVertices[ i ].x;
            sflVertexes.m_pData[ i * 3 + 1 ] = passimpLoadedMesh->mVertices[ i ].y;
            sflVertexes.m_pData[ i * 3 + 2 ] = passimpLoadedMesh->mVertices[ i ].z;
        }

        ::util::data::Span<int> snIndexes( passimpLoadedMesh->mNumFaces * 3, memory::alloc_nT<int>( passimpLoadedMesh->mNumFaces * 3 ) );
        for ( int i = 0; i < passimpLoadedMesh->mNumFaces; i++ )
        {
            assert::Release( passimpLoadedMesh->mFaces[ i ].mNumIndices == 3 );

            snIndexes.m_pData[ i * 3 ] = passimpLoadedMesh->mFaces[ i ].mIndices[ 0 ];
            snIndexes.m_pData[ i * 3 + 1 ] = passimpLoadedMesh->mFaces[ i ].mIndices[ 1 ];
            snIndexes.m_pData[ i * 3 + 2 ] = passimpLoadedMesh->mFaces[ i ].mIndices[ 2 ];
        }

        return renderer::UploadModel( &rendererState, sflVertexes, snIndexes );
    };

    renderer::R_UploadModel rangerUploadResult = fnLoadAssimpModel( "Quakeguy_Ranger_Rigged.obj" );
    renderer::RenderableObject rangerRenderableObject {
            .m_vPosition {},
            .m_vRotation {},

            .m_wgpuVertexBuffer = rangerUploadResult.m_wgpuVertexBuffer,
		    .m_nVertexBufferSize = rangerUploadResult.m_nVertexBufferSize,

		    .m_wgpuIndexBuffer = rangerUploadResult.m_wgpuIndexBuffer,
		    .m_nIndexBufferSize = rangerUploadResult.m_nIndexBufferSize,
		    .m_nIndexBufferCount = rangerUploadResult.m_nIndexBufferCount
        };

    renderer::R_UploadModel cubeUploadResult = fnLoadAssimpModel( "cube.obj" );
    renderer::RenderableObject cubeRenderableObject {
            .m_vPosition {},
            .m_vRotation {},

            .m_wgpuVertexBuffer = cubeUploadResult.m_wgpuVertexBuffer,
		    .m_nVertexBufferSize = cubeUploadResult.m_nVertexBufferSize,

		    .m_wgpuIndexBuffer = cubeUploadResult.m_wgpuIndexBuffer,
		    .m_nIndexBufferSize = cubeUploadResult.m_nIndexBufferSize,
		    .m_nIndexBufferCount = cubeUploadResult.m_nIndexBufferCount
        };

    renderer::RenderableObject * pRenderableObjects = memory::alloc_nT<renderer::RenderableObject>( 2 );
    pRenderableObjects[ 0 ] = rangerRenderableObject;
    pRenderableObjects[ 1 ] = cubeRenderableObject;

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

        renderer::Frame( &rendererState, util::data::Span<renderer::RenderableObject *>( 2, &pRenderableObjects ) );
    }

    util::commandline::Free( &caCommandLine );

    return 0;
}
