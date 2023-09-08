#include "game_loadassimp.hpp"

#include <assimp/material.h>
#include <libtitanium/memory/mem_core.hpp>
#include <libtitanium/util/data/span.hpp>
#include <libtitanium/util/assert.hpp>
#include <libtitanium/logger/logger.hpp>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/*
 *  Load a complete model using the assimp library
 */
renderer::GPUModelHandle Assimp_LoadScene( renderer::TitaniumRendererState *const pRendererState, const char *const pszModelName )
{
    const aiScene *const passimpLoadedModel = aiImportFile( pszModelName, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_Triangulate );
    logger::Info( "%s" ENDL, passimpLoadedModel != nullptr ? "Loaded model! c:" : "Didn't load model :c (we will probably crash now)" );

    logger::Info( "Model has %i meshes, we expect 1" ENDL, passimpLoadedModel->mNumMeshes );
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

    return renderer::UploadModel( pRendererState, sflVertexes, snIndexes );
}
