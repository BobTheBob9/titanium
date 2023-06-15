#pragma once

#include <SDL.h>
#include <SDL_syswm.h>
#include "extern/wgpu-native/wgpu.h"
#include "extern/wgpu-native/webgpu.h"

#include "titanium/util/data/staticspan.hpp"

namespace sys::platform::sdl
{
    void Initialise();
    WGPUSurface CreateWGPUSurfaceForWindow( SDL_Window *const psdlWindow, const WGPUInstance wgpuInstance );
}