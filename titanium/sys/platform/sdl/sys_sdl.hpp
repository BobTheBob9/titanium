#pragma once

#include <SDL.h>
#include <SDL_syswm.h>
#include <webgpu/webgpu.h>

#include "titanium/util/data/staticspan.hpp"

namespace sys::platform::sdl
{
    void Initialise();
    WGPUSurface CreateWGPUSurfaceForWindow( SDL_Window *const psdlWindow, const WGPUInstance wgpuInstance );
}