#pragma once

#include <SDL.h>
#include <SDL_syswm.h>
#include "extern/wgpu-native/wgpu.h"
#include "extern/wgpu-native/webgpu.h"

namespace renderer
{
	struct TitaniumPhysicalRenderingDevice
	{
		WGPUInstance m_wgpuInstance;
		WGPUAdapter m_wgpuAdapter;
	}; 

	/*
	 *  Runtime state of the renderer, initialised with renderer::Initialise
	 */
	struct TitaniumRendererState
	{
		SDL_Window * m_psdlWindow; // only 1 renderer per window, at the moment this seems sensible

		WGPUDevice m_wgpuVirtualDevice;
		WGPUSurface m_wgpuSurface;
		WGPUSwapChain m_wgpuSwapChain;
		WGPUQueue m_wgpuQueue;
		WGPURenderPipeline m_wgpuRenderPipeline;

		WGPUTextureView m_wgpuDepthTextureView;

		WGPUBuffer m_wgpuUniformBuffer;
		WGPUBindGroup m_wgpuUniformBindGroup;

		WGPUBuffer m_wgpuVertexBuffer;
		int m_nVertexBufferSize;

		WGPUBuffer m_wgpuIndexBuffer;
		int m_nIndexBufferSize;
		int m_nIndexBufferCount;

		int m_nFramesRendered;
	};

	/*
	 *  Initialises the application's handle to the physical rendering device, this should generally done once per application
	 */
	void InitialisePhysicalRenderingDevice( TitaniumPhysicalRenderingDevice *const pRendererDevice );

	/*
	 * 	Initialises (or re-initialises) the renderer
	 *  pRenderer can have been previously initialised, or never initialised before
	 *
	 *  If it was not previously initialised, it should be zeroed out (i.e. memset to 0)
	 */
	void Initialise( TitaniumPhysicalRenderingDevice *const pRendererDevice, TitaniumRendererState *const pRendererState, SDL_Window *const psdlWindow );

	namespace preframe
	{
		/*
		 *  Remakes swap chains etc for a new render resolution
		 */
    	void ResolutionChanged( TitaniumPhysicalRenderingDevice *const pRendererDevice, TitaniumRendererState *const pRendererState );

		/*
		 *  Calls the imgui preframe function for the current rendering api's imgui backend
		 */
		void ImGUI( TitaniumRendererState *const pRendererState );
	}

	/*
	 *  Renders a frame
	 */
	void Frame( TitaniumRendererState *const pRendererState );
};
