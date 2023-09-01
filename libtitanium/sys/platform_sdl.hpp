#pragma once

#include <SDL.h>
#include <SDL_syswm.h>
#include <webgpu/webgpu.h>

#include <libtitanium/util/maths.hpp>

namespace sys::sdl
{
    WGPUSurface CreateWGPUSurfaceForWindow( SDL_Window *const psdlWindow, const WGPUInstance wgpuInstance );
    util::maths::Vec2<u32> GetWindowSizeVector( SDL_Window *const psdlWindow );
}
