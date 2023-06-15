#include "renderer_api.hpp"

#include "titanium/sys/platform/sdl/sys_sdl.hpp"
#include "titanium/util/data/staticspan.hpp"
#include "titanium/logger/logger.hpp"
#include "titanium/renderer/utils/stringify.hpp"

namespace renderer
{
    void C_WGPUDeviceHandleError( const WGPUErrorType ewgpuErrorType, const char * const pszMessage, void *const pUserdata )
    {
        logger::Info( "%s: type: %s, %s" ENDL, __FUNCTION__, renderer::utils::wgpu::ErrorTypeToString( ewgpuErrorType ), pszMessage );
    }

    void Initialise( TitaniumRendererState *const pRendererState, SDL_Window *const psdlWindow )
    {
        logger::Info( "(Re?)Initialising wgpu renderer..." ENDL );
        logger::Info( "wgpu version is %i" ENDL, wgpuGetVersion() );

        const WGPUInstanceDescriptor wgpuDesc {};
        WGPUInstance wgpuInstance = wgpuCreateInstance( &wgpuDesc );

        pRendererState->m_Internal.m_psdlWindow = psdlWindow;
        WGPUSurface wgpuSurface = sys::platform::sdl::CreateWGPUSurfaceForWindow( pRendererState->m_Internal.m_psdlWindow, wgpuInstance );

        // Request graphics adapter from instance
        {
            const WGPURequestAdapterOptions wgpuAdapterOptions { 
                .compatibleSurface = wgpuSurface, // note: doesn't actually need a surface!
                .powerPreference = WGPUPowerPreference_HighPerformance
            };

            wgpuInstanceRequestAdapter( 
                wgpuInstance, 
                &wgpuAdapterOptions,  
                []( const WGPURequestAdapterStatus wgpuRequestAdapterStatus, WGPUAdapter wgpuAdapter, const char *const pszMessage, void *const pUserdata ) 
                {
                    logger::Info( "wgpuInstanceRequestAdapter returned %s, with message: %s" ENDL, renderer::utils::wgpu::RequestAdapterStatusToString( wgpuRequestAdapterStatus ), pszMessage );
                    
                    if ( wgpuRequestAdapterStatus == WGPURequestAdapterStatus_Success )
                    {
                       *reinterpret_cast<WGPUAdapter *>( pUserdata ) = wgpuAdapter;
                    }
               },
               &pRendererState->m_Internal.m_wgpuAdapter
            );
        }

        // Print info about the graphics adapter
        {
            WGPUAdapterProperties wgpuAdapterProperties;
            wgpuAdapterGetProperties( pRendererState->m_Internal.m_wgpuAdapter, &wgpuAdapterProperties );

            logger::Info( 
                "Found Adapter: %s %s, using backend %s" ENDL, 
                renderer::utils::wgpu::AdapterTypeToString( wgpuAdapterProperties.adapterType ), 
                wgpuAdapterProperties.name,

                renderer::utils::wgpu::BackendTypeToString( wgpuAdapterProperties.backendType ) 
            );

            ::utils::data::StaticSpan<WGPUFeatureName, 32> swgpuFeatures;
            size_t nFeatures = wgpuAdapterEnumerateFeatures( pRendererState->m_Internal.m_wgpuAdapter, swgpuFeatures.m_tData );

            if ( nFeatures )
            {
                logger::Info( "Adapter has features: " ENDL );

                for ( int i = 0; i < nFeatures; i++ )
                {
                    // TODO: track down why we're getting invalid enum values in this
                    logger::Info( "\t%s" ENDL, renderer::utils::wgpu::FeatureNameToString( swgpuFeatures.m_tData[ i ] ) );
                } 
            }

        }

        // Request graphics device from adapter
        {
            const WGPUDeviceDescriptor wgpuDeviceDescriptor {
                .requiredFeaturesCount = 0,
                .requiredLimits = nullptr,

                .defaultQueue {  }
            };

            wgpuAdapterRequestDevice(
                pRendererState->m_Internal.m_wgpuAdapter,
                &wgpuDeviceDescriptor,
                []( const WGPURequestDeviceStatus wgpuRequestDeviceStatus, WGPUDevice wgpuDevice, const char *const pszMessage, void *const pUserdata )
                {
                    logger::Info( "wgpuRequestDeviceStatus returned %s, with message: %s" ENDL, renderer::utils::wgpu::RequestDeviceStatusToString( wgpuRequestDeviceStatus ), pszMessage );
                    
                    if ( wgpuRequestDeviceStatus == WGPURequestDeviceStatus_Success )
                    {
                        *reinterpret_cast<WGPUDevice *>( pUserdata ) = wgpuDevice;
                    }
                },
                &pRendererState->m_Internal.m_wgpuDevice
            );

            wgpuDeviceSetUncapturedErrorCallback( pRendererState->m_Internal.m_wgpuDevice, C_WGPUDeviceHandleError, &pRendererState->m_Internal.m_wgpuDevice );
        }

        // Request queue from device
        pRendererState->m_Internal.m_wgpuQueue = wgpuDeviceGetQueue( pRendererState->m_Internal.m_wgpuDevice );

        WGPUTextureFormat wgpuSwapchainFormat = wgpuSurfaceGetPreferredFormat( wgpuSurface, pRendererState->m_Internal.m_wgpuAdapter );
        // Request swapchain from device
        {
            const WGPUSwapChainDescriptor wgpuSwapChainDescriptor {
                .usage = WGPUTextureUsage_RenderAttachment,
                .format = wgpuSwapchainFormat,

                .width = 1600,
                .height = 900,

                .presentMode = WGPUPresentMode_Fifo
            };

            pRendererState->m_Internal.m_wgpuSwapChain = wgpuDeviceCreateSwapChain( pRendererState->m_Internal.m_wgpuDevice, wgpuSurface, &wgpuSwapChainDescriptor );
        }

        // Create render pipeline for device
        {
            WGPUShaderModule wgpuShaderModule;
            {
                WGPUShaderModuleWGSLDescriptor wgpuShaderCodeDescriptor {
                    .chain { .sType = WGPUSType_ShaderModuleWGSLDescriptor },
                    .code = R"(
                                @vertex
                                fn vs_main( @builtin( vertex_index ) in_vertex_index: u32 ) -> @builtin( position ) vec4<f32> 
                                {
                                	var p = vec2<f32>( 0.0, 0.0 );

                                	if ( in_vertex_index == 0u ) 
                                    {
                                		p = vec2<f32>( -0.5, -0.5 );
                                	} 
                                    else if ( in_vertex_index == 1u ) 
                                    {
                                		p = vec2<f32>( 0.5, -0.5 );
                                	} 
                                    else if ( in_vertex_index == 2u ) 
                                    {
                                		p = vec2<f32>( -0.5, 0.5 );
                                	}

                                	return vec4<f32>( p, 0.0, 1.0 );
                                }

                                @fragment
                                fn fs_main() -> @location( 0 ) vec4<f32> 
                                {
                                    return vec4<f32>( 0.0, 0.4, 1.0, 0.2 );
                                }
                            )"
                };

                WGPUShaderModuleDescriptor wgpuShaderDescriptor {
                    .nextInChain = &wgpuShaderCodeDescriptor.chain
                };

                wgpuShaderModule = wgpuDeviceCreateShaderModule( pRendererState->m_Internal.m_wgpuDevice, &wgpuShaderDescriptor );
            }

            WGPUBlendState wgpuBlendState {
                .color {
                    .operation = WGPUBlendOperation_Add,
                    .srcFactor = WGPUBlendFactor_SrcAlpha,
                    .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha
                }
            };

            WGPUColorTargetState wgpuColourTargetState {
                .format = wgpuSwapchainFormat,
                .blend = &wgpuBlendState,
                .writeMask = WGPUColorWriteMask_All
            };

            WGPUFragmentState wgpuFragmentState {
                .module = wgpuShaderModule,
                .entryPoint = "fs_main",
                .constantCount = 0,
                .targetCount = 1,
                .targets = &wgpuColourTargetState
            };

            WGPURenderPipelineDescriptor wgpuRenderPipelineDescriptor {
                .vertex {
                    .module = wgpuShaderModule,
                    .entryPoint = "vs_main",
                    .constantCount = 0,
                    .bufferCount = 0,
                },

                .primitive {
                    .topology = WGPUPrimitiveTopology_TriangleList,
                    .stripIndexFormat = WGPUIndexFormat_Undefined,
                    .frontFace = WGPUFrontFace_CCW,
                    .cullMode = WGPUCullMode_None
                },

                .multisample {
                    .count = 1, // disable multisampling
                    .mask = ~0u,
                    .alphaToCoverageEnabled = false
                },

                .fragment = &wgpuFragmentState// this sucks
            };

            pRendererState->m_Internal.m_wgpuRenderPipeline = wgpuDeviceCreateRenderPipeline( pRendererState->m_Internal.m_wgpuDevice, &wgpuRenderPipelineDescriptor );
        }


        logger::Info( "wgpu renderer initialised successfully!" ENDL );
    }

    void Frame( TitaniumRendererState *const pRendererState )
    {
        WGPUTextureView wgpuNextTexture = wgpuSwapChainGetCurrentTextureView( pRendererState->m_Internal.m_wgpuSwapChain );

        WGPUCommandEncoderDescriptor wgpuCommandEncoderDescriptor {};
        WGPUCommandEncoder wgpuCommandEncoder = wgpuDeviceCreateCommandEncoder( pRendererState->m_Internal.m_wgpuDevice, &wgpuCommandEncoderDescriptor );

            // describe a render pass
        WGPURenderPassColorAttachment wgpuRenderPassColourAttachment {
            .view = wgpuNextTexture,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue { 0.9, 0.1, 0.2, 1.0 },
        };
 
        WGPURenderPassDescriptor wgpuRenderPassDescriptor {
            .colorAttachmentCount = 1,
            .colorAttachments = &wgpuRenderPassColourAttachment,
        };

        WGPURenderPassEncoder wgpuRenderPass = wgpuCommandEncoderBeginRenderPass( wgpuCommandEncoder, &wgpuRenderPassDescriptor );
        {
            // Select render pipeline
            wgpuRenderPassEncoderSetPipeline( wgpuRenderPass, pRendererState->m_Internal.m_wgpuRenderPipeline );
            wgpuRenderPassEncoderDraw( wgpuRenderPass, 3, 1, 0, 0 );
        }
        wgpuRenderPassEncoderEnd( wgpuRenderPass );

        wgpuTextureViewDrop( wgpuNextTexture );

        WGPUCommandBufferDescriptor wgpuCommandBufferDescriptor {};
        WGPUCommandBuffer wgpuCommand = wgpuCommandEncoderFinish( wgpuCommandEncoder, &wgpuCommandBufferDescriptor );

        wgpuQueueSubmit( pRendererState->m_Internal.m_wgpuQueue, 1, &wgpuCommand );
        wgpuSwapChainPresent( pRendererState->m_Internal.m_wgpuSwapChain ); 

        pRendererState->m_Internal.m_nFramesRendered++;
    }
}
