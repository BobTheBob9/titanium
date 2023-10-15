#include "game_loadassimp.hpp"



#include <assimp/material.h>
#include <assimp/types.h>
#include <stdio.h>
#include <libtitanium/memory/mem_core.hpp>
#include <libtitanium/util/data/span.hpp>
#include <libtitanium/util/assert.hpp>
#include <libtitanium/logger/logger.hpp>
#include <libtitanium/renderer/renderer.hpp>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <webgpu/webgpu.h>

#include <libtitanium/memory/mem_external.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

/*
 *  Load a complete model using the assimp library
 *  TODO: needs to support loading multiple models
 */
bool Assimp_LoadScene( renderer::TitaniumRendererState *const pRendererState, const char *const pszModelName, renderer::GPUModelHandle * o_gpuLoadedModel, util::data::Span<renderer::GPUTextureHandle> o_sgpuLoadedTextures )
{
    logger::Info( "Loading assimp scene model %s" ENDL, pszModelName );
    const aiScene *const passimpLoadedScene = aiImportFile( pszModelName, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_Triangulate | aiProcess_FlipUVs );

    if ( !passimpLoadedScene )
    {
        logger::Info( "Model load failed :c" ENDL );
        return false;
    }

    if ( passimpLoadedScene->mNumMeshes > 1 )
    {
        logger::Info( "We expect 1 mesh per scene, but this scene has more! truncating." ENDL );
    }

    const aiMesh *const passimpLoadedMesh = passimpLoadedScene->mMeshes[ 0 ];
    logger::Info( "\tModel has %i vertices and %i faces" ENDL, passimpLoadedMesh->mNumVertices, passimpLoadedMesh->mNumFaces );

    ::util::data::Span<renderer::ModelVertexAttributes> sflVertexes( passimpLoadedMesh->mNumVertices, memory::alloc_nT<renderer::ModelVertexAttributes>( passimpLoadedMesh->mNumVertices ) );
    for ( uint i = 0; i < passimpLoadedMesh->mNumVertices; i++ )
    {
        sflVertexes.pData[ i ] = {
            .vPosition { .x = passimpLoadedMesh->mVertices[ i ].x, .y = passimpLoadedMesh->mVertices[ i ].y, .z = passimpLoadedMesh->mVertices[ i ].z },
            .vTextureCoordinates = { .x = passimpLoadedMesh->mTextureCoords[ 0 ][ i ].x, .y = passimpLoadedMesh->mTextureCoords[ 0 ][ i ].y }
        };
    }

    ::util::data::Span<u16> snIndexes( passimpLoadedMesh->mNumFaces * 3, memory::alloc_nT<u16>( passimpLoadedMesh->mNumFaces * 3 ) );
    for ( uint i = 0; i < passimpLoadedMesh->mNumFaces; i++ )
    {
        if ( passimpLoadedMesh->mFaces[ i ].mNumIndices != 3 ) [[unlikely]]
        {
            logger::Info( "Model has weird number of face indices for face %i, not loading..." ENDL, passimpLoadedMesh->mFaces[ i ].mNumIndices );
            return false;
        }

        snIndexes.pData[ i * 3 ] = passimpLoadedMesh->mFaces[ i ].mIndices[ 0 ];
        snIndexes.pData[ i * 3 + 1 ] = passimpLoadedMesh->mFaces[ i ].mIndices[ 1 ];
        snIndexes.pData[ i * 3 + 2 ] = passimpLoadedMesh->mFaces[ i ].mIndices[ 2 ];
    }

    *o_gpuLoadedModel = renderer::UploadModel( pRendererState, sflVertexes, snIndexes );
    memory::free( sflVertexes.pData );
    memory::free( snIndexes.pData );

    uint nGlobalTextureIndex = 0;
    for ( uint i = 0; i < passimpLoadedScene->mNumMaterials; i++ )
    {
        int nTextureIndex = 0;
        aiString path;
        while ( passimpLoadedScene->mMaterials[ i ]->GetTexture( aiTextureType_BASE_COLOR, nTextureIndex++, &path ) == aiReturn_SUCCESS )
        {
            logger::Info( "\tModel texture: %s" ENDL, path.C_Str() );

            /*util::maths::Vec2<i32> vImageSize;
            const byte *const pImageData = stbi_load( path.C_Str(), &vImageSize.x, &vImageSize.y, nullptr, 4 );
            o_sgpuLoadedTextures.pData[ nGlobalTextureIndex++ ] = renderer::UploadTexture( pRendererState, { .x = static_cast<u16>( vImageSize.x ), .y = static_cast<u16>( vImageSize.y ) }, WGPUTextureFormat_RGBA8Unorm, pImageData );
            stbi_image_free( (void*)pImageData );*/
        }
    }

    util::maths::Vec2<i32> vImageSize;
    const byte *const pImageData = stbi_load( "test_resource/damaged/Default_albedo.tga", &vImageSize.x, &vImageSize.y, nullptr, 4 );
    o_sgpuLoadedTextures.pData[ nGlobalTextureIndex++ ] = renderer::UploadTexture( pRendererState, { .x = static_cast<u16>( vImageSize.x ), .y = static_cast<u16>( vImageSize.y ) }, WGPUTextureFormat_RGBA8Unorm, pImageData );
    stbi_image_free( (void*)pImageData );

    /*// TODO: TEMP!!!
    const aiTexture *const passimpTexture = passimpLoadedScene->GetEmbeddedTexture( "*0" );
    logger::Info( "Height: %i, Width: %i" ENDL, passimpTexture->mHeight, passimpTexture->mWidth );

    if ( !passimpTexture->mHeight ) // compressed :c
    {
        logger::Info( "Compressed texture with format %s" ENDL, passimpTexture->achFormatHint );
    }*/

    aiReleaseImport( passimpLoadedScene );
    return true;
}
