#pragma once

#include <SDL.h>
#include <webgpu/webgpu.h>

#include <libtitanium/util/maths.hpp>
#include <libtitanium/util/data/span.hpp>

namespace renderer
{
	struct TitaniumPhysicalRenderingDevice
	{
		WGPUInstance m_wgpuInstance;
		WGPUAdapter m_wgpuAdapter;
	}; 

	struct DepthTextureAndView
	{
		WGPUTexture m_wgpuDepthTexture;
		WGPUTextureView m_wgpuDepthTextureView;
	};

	struct BufferAndBindgroup
	{
		WGPUBindGroup m_wgpuBindGroup;
		WGPUBuffer m_wgpuBuffer;
	};

	/*
	 *  Runtime state of the renderer, initialised with renderer::Initialise
	 */
	struct TitaniumRendererState
	{
		util::maths::Vec2<u32> m_vWindowSize;

		WGPUDevice m_wgpuVirtualDevice;
		WGPUSurface m_wgpuRenderSurface;
		WGPUSwapChain m_wgpuSwapChain;
		WGPUQueue m_wgpuQueue;
		WGPURenderPipeline m_wgpuRenderPipeline;

		DepthTextureAndView m_depthTextureAndView;

		BufferAndBindgroup m_globalUniformBuffer;

		WGPUBindGroupLayout m_wgpuStandardObjectUniformBindGroupLayout;
		
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

	struct GPUModelHandle
	{
		WGPUBuffer m_wgpuVertexBuffer;
		size_t m_nVertexBufferSize;

		WGPUBuffer m_wgpuIndexBuffer;
		size_t m_nIndexBufferSize;
		int m_nIndexBufferCount;
	};
	
	GPUModelHandle UploadModel( TitaniumRendererState *const pRendererState, const ::util::data::Span<float> sflVertices, const ::util::data::Span<int> snIndexes );
	void FreeGPUModel( GPUModelHandle gpuModel );

	/*
	 *  An object that can be rendered by the renderer
	 */
	struct RenderableObject
	{
		util::maths::Vec3<f32> m_vPosition;
		util::maths::Vec3<f32> m_vRotation;

		BufferAndBindgroup m_standardUniforms;
		GPUModelHandle m_gpuModel;
	};

    void CreateRenderableObjectBuffers( TitaniumRendererState *const pRendererState, RenderableObject *const pRenderableObject );
    void FreeRenderableObjectBuffers( RenderableObject *const pRenderableObject );

	namespace preframe
	{
		/*
		 *  Remakes swap chains etc for a new render resolution
		 */
    	void ResolutionChanged( TitaniumPhysicalRenderingDevice *const pRendererDevice, TitaniumRendererState *const pRendererState, const ::util::maths::Vec2<u32> vWindowSize );

		/*
		 *  Calls the imgui preframe function for the current rendering api's imgui backend
		 */
		void ImGUI( TitaniumRendererState *const pRendererState );
	}

	/*
	 *  Renders a frame
	 */
	// TODO: should have float flCurrentTime parameter
	void Frame( TitaniumRendererState *const pRendererState, const ::util::data::Span<RenderableObject> sRenderableObjects );
};
