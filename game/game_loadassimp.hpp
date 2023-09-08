#pragma once

#include <libtitanium/renderer/renderer.hpp>

renderer::GPUModelHandle Assimp_LoadScene( renderer::TitaniumRendererState *const pRendererState, const char *const pszModelName );
