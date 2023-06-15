#pragma once

#include <SDL.h>
#include <SDL_syswm.h>
#include "extern/wgpu-native/wgpu.h"
#include "extern/wgpu-native/webgpu.h"

namespace renderer
{
	struct TitaniumRendererState
	{
		struct {
			SDL_Window * m_psdlWindow;

			WGPUAdapter m_wgpuAdapter;
			WGPUDevice m_wgpuDevice;
			WGPUSwapChain m_wgpuSwapChain;
			WGPUQueue m_wgpuQueue;
			WGPURenderPipeline m_wgpuRenderPipeline;

			int m_nFramesRendered;
		} m_Internal;
	};

	/*
	 * 	Initialises (or re-initialises) the renderer
	 *  pRenderer can have been previously initialised, or never initialised before
	 *
	 *  If it was not previously initialised, it should be zeroed out (i.e. memset to 0)
	 */
	void Initialise( TitaniumRendererState *const pRendererState, SDL_Window *const psdlWindow );

	/*
	 *  Renders a frame
	 */
	void Frame( TitaniumRendererState *const pRendererState );
};
