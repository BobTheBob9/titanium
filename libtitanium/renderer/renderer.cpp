#include "renderer.hpp"
#include "renderer_uniforms.hpp"

#include "extern/imgui/imgui.h"
#include "extern/imgui/imgui_impl_wgpu.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <libtitanium/util/numerics.hpp>
#include <libtitanium/util/maths.hpp>
#include <libtitanium/sys/platform_sdl.hpp>
#include <libtitanium/util/data/staticspan.hpp>
#include <libtitanium/logger/logger.hpp>
#include <libtitanium/config/config.hpp>
#include <libtitanium/renderer/renderer_stringify.hpp>

#include <chrono> // temp probably, idk if we wanna use this for time
#include <webgpu/webgpu.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/euler_angles.hpp>


// TODO: the renderer is super complex and will only get more complex, it should really be split into multiple files
// atm, code is a bit unwieldly, hard to navigate and understand at a glance which sucks

namespace renderer
{
    config::Var<bool> * g_pbcvarPreferImmediatePresent = config::RegisterVar<bool>( "renderer:preferimmediatepresent", false, config::EFVarUsageFlags::NONE );
    config::Var<bool> * g_pbcvarShowFps = config::RegisterVar<bool>( "renderer:showfps", false, config::EFVarUsageFlags::NONE );
    
    void C_WGPUVirtualDeviceHandleUncaughtError( const WGPUErrorType ewgpuErrorType, const char * const pszMessage, void *const pUserdata )
    {
        logger::Info( "%s: type: %s, %s" ENDL, __FUNCTION__, WGPUErrorTypeToString( ewgpuErrorType ), pszMessage );
    }

    WGPUSwapChain CreateSwapChainForWindowDimensions( TitaniumPhysicalRenderingDevice *const pRendererDevice, TitaniumRendererState *const pRendererState, const util::maths::Vec2<u32> vWindowSize )
    {
        // get supported surface present modes, ideally take the preferred one, but if that's not available, get the default supported
        WGPUPresentMode wgpuPreferredPresentMode = g_pbcvarPreferImmediatePresent->tValue ? WGPUPresentMode_Immediate : WGPUPresentMode_Fifo; // TODO: make configurable
        const WGPUPresentMode wgpuPresentMode = wgpuPreferredPresentMode; 

        // TODO: this is nonstandard, need a proper way to do it
        /*const WGPUPresentMode wgpuPresentMode = [ wgpuSurface, wgpuAdapter, wgpuPreferredPresentMode ](){
            // wgpuSurfaceGetCapabilities function populates user-provided pointers in the struct passed to it
            // so we need to provide a buffer for all the array fields we want to query before calling (in this case, presentModes)
            // TODO: remove magic number, this is the number of entries in the WGPUPresentMode enum

            // TODO: nonstandard
            /util::data::StaticSpan<WGPUPresentMode, 4> sPresetModes;
            WGPUSurfaceCapabilities wgpuSurfaceCapabilities { 
                .presentModeCount = sPresetModes.Elements(), // max size of the buffer
                .presentModes = sPresetModes.m_tData 
            }; 
            wgpuSurfaceGetCapabilities( wgpuSurface, wgpuAdapter, &wgpuSurfaceCapabilities );

            // we want WGPUPresentMode_Fifo for vsync, or WGPUPresentMode_Immediate for non-vsync
            for ( int i = 0; i < wgpuSurfaceCapabilities.presentModeCount; i++ )
            {
                if ( wgpuSurfaceCapabilities.presentModes[ i ] == wgpuPreferredPresentMode )
                {
                    return wgpuPreferredPresentMode;
                }
            }

            // i *believe* it is required for fifo to be supported (this is the case in vulkan) so default to it
            // TODO reexamine this, unsure if it's right
            return WGPUPresentMode_Immediate; 
        }();*/

        WGPUTextureFormat wgpuSwapchainFormat;
#if WEBGPU_BACKEND_WGPU 
        wgpuSwapchainFormat = wgpuSurfaceGetPreferredFormat( pRendererState->m_wgpuRenderSurface, pRendererDevice->m_wgpuGraphicsAdapter );
#elif WEBGPU_BACKEND_DAWN
        wgpuSwapchainFormat = WGPUTextureFormat_BGRA8Unorm; // dawn only supports this, and doesn't have wgpuSurfaceGetPreferredFormat
#endif 

        const WGPUSwapChainDescriptor wgpuSwapChainDescriptor {
            .usage = WGPUTextureUsage_RenderAttachment,
            .format = wgpuSwapchainFormat,

            .width = vWindowSize.x,
            .height = vWindowSize.y,

            .presentMode = wgpuPresentMode 
        };

        // TODO: this is a bit spammy, add when we have designated spam logs
        //logger::Info( "Creating swapchain: width %i, height %i present mode: %s" ENDL, nWindowWidth, nWindowHeight, WGPUPresentModeToString( wgpuPresentMode ) );
        return wgpuDeviceCreateSwapChain( pRendererState->m_wgpuVirtualDevice, pRendererState->m_wgpuRenderSurface, &wgpuSwapChainDescriptor );
    }

    DepthTextureAndView CreateDepthTextureAndViewForWindowSize( TitaniumRendererState *const pRendererState, const util::maths::Vec2<u32> vWindowSize )
    {
        // make depth buffer
        // TODO: const or unify between depthstencilstate and here somehow
        //
        static const WGPUTextureFormat wgpuDepthTextureFormat = WGPUTextureFormat_Depth24Plus; 
        WGPUTextureDescriptor wgpuDepthTextureDescriptor {
            .usage = WGPUTextureUsage_RenderAttachment,
            .dimension = WGPUTextureDimension_2D,
            .size = { vWindowSize.x, vWindowSize.y, 1 },
            .format = WGPUTextureFormat_Depth24Plus, 
            .mipLevelCount = 1,
            .sampleCount = 1,
            .viewFormatCount = 1,
            .viewFormats = &wgpuDepthTextureFormat
        };

        // TODO: we probably aren't cleaning this up...
        WGPUTexture wgpuDepthTexture = wgpuDeviceCreateTexture( pRendererState->m_wgpuVirtualDevice, &wgpuDepthTextureDescriptor );
    
        WGPUTextureViewDescriptor wgpuDepthTextureViewDescriptor {
            .format = WGPUTextureFormat_Depth24Plus,
            .dimension = WGPUTextureViewDimension_2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
            .aspect = WGPUTextureAspect_DepthOnly
        };
        WGPUTextureView r_wgpuDepthTextureView = wgpuTextureCreateView( wgpuDepthTexture, &wgpuDepthTextureViewDescriptor );

        return {
            .m_wgpuDepthTexture = wgpuDepthTexture, 
            .m_wgpuDepthTextureView = r_wgpuDepthTextureView
        };
    }

    void FreeDepthTextureAndView( DepthTextureAndView *const pDepthTexture )
    {
        wgpuTextureDestroy( pDepthTexture->m_wgpuDepthTexture );
        wgpuTextureRelease( pDepthTexture->m_wgpuDepthTexture );
        wgpuTextureViewRelease( pDepthTexture->m_wgpuDepthTextureView );
    }

    WGPUBindGroupLayout CreateBindGroupLayout( TitaniumRendererState *const pRendererState, util::data::Span<WGPUBindGroupLayoutEntry> swgpuBindGroupLayoutEntries )
    {
        WGPUBindGroupLayoutDescriptor wgpuUniformBindGroupLayoutDescriptor {
            .entryCount = static_cast<u32>( swgpuBindGroupLayoutEntries.m_nElements ),
            .entries = swgpuBindGroupLayoutEntries.m_pData
        };

        return wgpuDeviceCreateBindGroupLayout( pRendererState->m_wgpuVirtualDevice, &wgpuUniformBindGroupLayoutDescriptor );
    }



    void Initialise( TitaniumPhysicalRenderingDevice *const pRendererDevice, TitaniumRendererState *const pRendererState, SDL_Window *const psdlWindow )
    {
        logger::Info( "Initialising wgpu renderer..." ENDL );

        WGPUAdapter wgpuGraphicsAdapter = pRendererDevice->m_wgpuGraphicsAdapter;

        const util::maths::Vec2<u32> vWindowSize = sys::sdl::GetWindowSizeVector( psdlWindow );
        WGPUSurface wgpuSurface = pRendererState->m_wgpuRenderSurface = sys::sdl::CreateWGPUSurfaceForWindow( psdlWindow, pRendererDevice->m_wgpuInstance );

        WGPUTextureFormat wgpuSwapchainFormat;
#if WEBGPU_BACKEND_WGPU 
        wgpuSwapchainFormat = wgpuSurfaceGetPreferredFormat( wgpuSurface, wgpuGraphicsAdapter );
#elif WEBGPU_BACKEND_DAWN
        wgpuSwapchainFormat = WGPUTextureFormat_BGRA8Unorm; // dawn only supports this, and doesn't have wgpuSurfaceGetPreferredFormat
#endif 

        // Request virtual graphics device from physical adapter
        WGPUDevice wgpuVirtualDevice = pRendererState->m_wgpuVirtualDevice = [ wgpuGraphicsAdapter ]()
        {
            const WGPUDeviceDescriptor wgpuVirtualDeviceDescriptor { };
            WGPUDevice r_wgpuVirtualDevice;
            wgpuAdapterRequestDevice(
                wgpuGraphicsAdapter,
                &wgpuVirtualDeviceDescriptor,
                []( const WGPURequestDeviceStatus wgpuRequestDeviceStatus, WGPUDevice wgpuVirtualDevice, const char *const pszMessage, void *const pUserdata )
                {
                    logger::Info( "wgpuRequestDeviceStatus returned %s, with message: %s" ENDL, WGPURequestDeviceStatusToString( wgpuRequestDeviceStatus ), pszMessage );
                    
                    if ( wgpuRequestDeviceStatus == WGPURequestDeviceStatus_Success )
                    {
                        *reinterpret_cast<WGPUDevice *>( pUserdata ) = wgpuVirtualDevice;
                    }
                },
                &r_wgpuVirtualDevice
            );

            return r_wgpuVirtualDevice;
        }();

        wgpuDeviceSetUncapturedErrorCallback( wgpuVirtualDevice, C_WGPUVirtualDeviceHandleUncaughtError, &wgpuVirtualDevice );

        WGPUQueue wgpuQueue = pRendererState->m_wgpuQueue = wgpuDeviceGetQueue( wgpuVirtualDevice );
        pRendererState->m_wgpuSwapChain = CreateSwapChainForWindowDimensions( pRendererDevice, pRendererState, vWindowSize );

        ImGui_ImplWGPU_Init( wgpuVirtualDevice, 1, wgpuSwapchainFormat, WGPUTextureFormat_Depth24Plus );


        /*
         *  We're done with webgpu global inits
         *  So, let's start doing stuff specific to our renderer: creating base uniform structures, pipelines and global buffers
         */


        // create error scope for pipeline creation so we can react to it
        wgpuDevicePushErrorScope( wgpuVirtualDevice, WGPUErrorFilter_Validation );

        // create builtin uniform bindgroup layouts, this defines the way uniforms are laid out in the render pipeline
        WGPUBindGroupLayout wgpuUniformBindGroupLayout_UShaderView = pRendererState->m_wgpuUniformBindGroupLayout_UShaderView = CreateBindGroupLayout( pRendererState, util::data::StaticSpan<WGPUBindGroupLayoutEntry, 1> {
            {
                .binding = 0,
                .visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Vertex,

                .buffer {
                    .type = WGPUBufferBindingType_Uniform,
                    .minBindingSize = sizeof( UShaderView )
                }
            }
        }.ToConstSpan() );

        WGPUBindGroupLayout wgpuUniformBindGroupLayout_UShaderObjectInstance = pRendererState->m_wgpuUniformBindGroupLayout_UShaderObjectInstance = CreateBindGroupLayout( pRendererState, util::data::StaticSpan<WGPUBindGroupLayoutEntry, 1> {
            {
                .binding = 0,
                .visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Vertex,

                .buffer {
                    .type = WGPUBufferBindingType_Uniform,
                    .minBindingSize = sizeof( UShaderObjectInstance )
                }
            }
        }.ToConstSpan() );

        // Create base render pipeline, with a basic shader that uses the global uniform buffer, and the per-object uniform buffer
        pRendererState->m_wgpuObjectRenderPipeline = [ wgpuVirtualDevice, wgpuSwapchainFormat, wgpuUniformBindGroupLayout_UShaderView, wgpuUniformBindGroupLayout_UShaderObjectInstance ]()
        {
            // Create shader module
            WGPUShaderModule wgpuShaderModule = [ wgpuVirtualDevice ]()
            {
                // TODO: need projection matrices in uniforms
                WGPUShaderModuleWGSLDescriptor wgpuShaderCodeDescriptor {
                    .chain { .sType = WGPUSType_ShaderModuleWGSLDescriptor },
                    .code = R"(
                                const PI = 3.14159265359;

                                struct UShaderView
                                {
                                    mat4fCameraTransform : mat4x4<f32>,
                                    vWindowSize : vec2<u32>
                                };

                                struct UShaderObjectInstance
                                {
                                    mat4fBaseTransform : mat4x4<f32>
                                };

                                @group( 0 ) @binding( 0 ) var<uniform> u_view : UShaderView;
                                @group( 1 ) @binding( 0 ) var<uniform> u_object : UShaderObjectInstance;

                                struct R_VertexOutput
                                {
                                    @builtin( position ) position : vec4<f32>,
                                    @location( 0 ) colour : vec3<f32>
                                };

                                @vertex fn vs_main( @location( 0 ) vertexPosition : vec3<f32> ) -> R_VertexOutput
                                {
                                    var r_out : R_VertexOutput;
                                    r_out.position = u_view.mat4fCameraTransform * u_object.mat4fBaseTransform * vec4<f32>( vertexPosition, 1.0 );
                                    r_out.colour = vec3<f32>( 1.0, 1.0, 1.0 ); // r_out.position.xyz * cos( u_view.flTime );

                                    return r_out;
                                }

                                @fragment fn fs_main( in : R_VertexOutput ) -> @location( 0 ) vec4<f32>
                                {
                                    return vec4<f32>( in.position.xyz, 1.0 ); // vec4<f32>( pow( in.colour + ( sin( u_view.flTime ) * 0.4 ), vec3<f32>( 2.2 ) ), 1.0 );
                                }
                            )"
                };

                WGPUShaderModuleDescriptor wgpuShaderDescriptor {
                    .nextInChain = &wgpuShaderCodeDescriptor.chain
                };

                return wgpuDeviceCreateShaderModule( wgpuVirtualDevice, &wgpuShaderDescriptor );
            }();

            util::data::StaticSpan<WGPUBindGroupLayout, 2> swgpuUniformBindGroupLayouts {
                wgpuUniformBindGroupLayout_UShaderView,
                wgpuUniformBindGroupLayout_UShaderObjectInstance
            };

            // create pipeline layout
            WGPUPipelineLayoutDescriptor wgpuPipelineLayoutDescriptor {
                .bindGroupLayoutCount = static_cast<u32>( swgpuUniformBindGroupLayouts.Elements() ),
                .bindGroupLayouts = swgpuUniformBindGroupLayouts.m_tData
            };
            WGPUPipelineLayout wgpuPipelineLayout =  wgpuDeviceCreatePipelineLayout( wgpuVirtualDevice, &wgpuPipelineLayoutDescriptor );

            util::data::StaticSpan<WGPUVertexAttribute, 1> sVertexAttributes {
                // position attribute
                {
                    .format = WGPUVertexFormat_Float32x3,
                    .offset = 0,
                    .shaderLocation = 0
                },
            };
            WGPUVertexBufferLayout wgpuVertexBufferLayout {
                .arrayStride = sizeof( float ) * 3,
                .stepMode = WGPUVertexStepMode_Vertex,
                .attributeCount = static_cast<u32>( sVertexAttributes.Elements() ),
                .attributes = sVertexAttributes.m_tData
            };

            WGPUDepthStencilState wgpuDepthStencilOperations {
                .format = WGPUTextureFormat_Depth24Plus,
                .depthWriteEnabled = true,
                .depthCompare = WGPUCompareFunction_Less,

                .stencilFront = { .compare = WGPUCompareFunction_Always },
                .stencilBack = { .compare = WGPUCompareFunction_Always },
                .stencilReadMask = 0xFFFFFFFF,
                .stencilWriteMask = 0xFFFFFFFF
            };

            WGPUBlendState wgpuBlendOperations {
                .color {
                    .operation = WGPUBlendOperation_Add,
                    .srcFactor = WGPUBlendFactor_SrcAlpha,
                    .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha
                }
            };

            WGPUColorTargetState wgpuColourTargetOperations {
                .format = wgpuSwapchainFormat,
                .blend = &wgpuBlendOperations,
                .writeMask = WGPUColorWriteMask_All
            };

            WGPUFragmentState wgpuFragmentOperations {
                .module = wgpuShaderModule,
                .entryPoint = "fs_main",
                .constantCount = 0,
                .targetCount = 1,
                .targets = &wgpuColourTargetOperations
            };

            WGPURenderPipelineDescriptor wgpuRenderPipelineDescriptor {
                .layout = wgpuPipelineLayout,

                .vertex {
                    .module = wgpuShaderModule,
                    .entryPoint = "vs_main",
                    .constantCount = 0,
                    .bufferCount = 1,
                    .buffers = &wgpuVertexBufferLayout
                },

                .primitive {
                    .topology = WGPUPrimitiveTopology_TriangleList,
                    .frontFace = WGPUFrontFace_CCW,
                    .cullMode = WGPUCullMode_Back
                },

                .depthStencil = &wgpuDepthStencilOperations,

                .multisample {
                    .count = 1, // disable multisampling
                    .mask = ~0u,
                    .alphaToCoverageEnabled = false
                },

                .fragment = &wgpuFragmentOperations // this sucks, wish there was a way of aggregate-initialising this inline
            };

            return wgpuDeviceCreateRenderPipeline( wgpuVirtualDevice, &wgpuRenderPipelineDescriptor );
        }();

        bool bPipelineCompilationFailed = false;
        wgpuDevicePopErrorScope( wgpuVirtualDevice, []( const WGPUErrorType ewgpuErrorType, const char * const pszMessage, void *const bPipelineCompilationFailed ){ 
            logger::Info( "Pipeline creation failed with message: %s" ENDL, pszMessage );
            *static_cast<bool *>( bPipelineCompilationFailed ) = true;
        }, &bPipelineCompilationFailed );

        // TODO: should this be a part of the RenderView?
        pRendererState->m_depthTextureAndView = CreateDepthTextureAndViewForWindowSize( pRendererState, vWindowSize );
        logger::Info( "wgpu renderer initialised successfully!" ENDL );
    }



    void preframe::ResolutionChanged( TitaniumPhysicalRenderingDevice *const pRendererDevice, TitaniumRendererState *const pRendererState, RenderView *const pRenderView, const util::maths::Vec2<u32> vWindowSize )
    {
        // swapchains rely on the window's resolution, so need to be recreated on window resize
        wgpuSwapChainRelease( pRendererState->m_wgpuSwapChain ); // destroy old swapchain
        pRendererState->m_wgpuSwapChain = CreateSwapChainForWindowDimensions( pRendererDevice, pRendererState, vWindowSize );

        FreeDepthTextureAndView( &pRendererState->m_depthTextureAndView );
        pRendererState->m_depthTextureAndView = CreateDepthTextureAndViewForWindowSize( pRendererState, vWindowSize );

        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, pRenderView->m_viewUniforms.m_wgpuBuffer, offsetof( UShaderView, m_vWindowSize ), &vWindowSize, sizeof( util::maths::Vec2<u32> ) );
    }

    void preframe::ImGUI( TitaniumRendererState *const pRendererState )
    {
        ImGui_ImplWGPU_NewFrame();
    }



    void Frame( TitaniumRendererState *const pRendererState, RenderView *const pRenderView, const util::data::Span<RenderableObject> sRenderableObjects )
    {
#if WEBGPU_BACKEND_WGPU 
        // TODO: what do i actually need here??
        //wgpuDevicePoll( pRendererState->m_wgpuVirtualDevice );
#elif WEBGPU_BACKEND_DAWN
        wgpuDeviceTick( pRendererState->m_wgpuVirtualDevice );
#endif 

        // write view state to view uniform if view state has changed
        if ( pRenderView->m_bGPUDirty )
        {
            glm::mat4x4 mat4fViewTransform = glm::eulerAngleXYZ( pRenderView->m_vCameraRotation.x, pRenderView->m_vCameraRotation.y, pRenderView->m_vCameraRotation.z );
            mat4fViewTransform = glm::translate( mat4fViewTransform, glm::vec3( pRenderView->m_vCameraPosition.x, pRenderView->m_vCameraPosition.y, pRenderView->m_vCameraPosition.z ) );
            mat4fViewTransform = glm::transpose( mat4fViewTransform );

            constexpr float FOV_90_DEG_IN_RADS = 1.5; // TODO: temp
            constexpr float NEAR_DIST = 0.01;
            constexpr float FAR_DIST = 1000.0;
            glm::mat4x4 mat4fFocalProjection = glm::perspective( FOV_90_DEG_IN_RADS, (float)pRenderView->m_vRenderResolution.x / pRenderView->m_vRenderResolution.y, NEAR_DIST, FAR_DIST );
            mat4fFocalProjection = glm::transpose( mat4fFocalProjection );

            const glm::mat4x4 mat4fCameraTransform = mat4fFocalProjection * mat4fViewTransform;
            wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, pRenderView->m_viewUniforms.m_wgpuBuffer, offsetof( UShaderView, m_mat4fCameraTransform ), &mat4fCameraTransform, sizeof( mat4fCameraTransform ) );

            pRenderView->m_bGPUDirty = false;
        }

        static auto programTimeBegin = std::chrono::high_resolution_clock::now();
        static float fsecFrameDiff = 0.0f;

        auto timeBegin = std::chrono::high_resolution_clock::now();

        if ( g_pbcvarShowFps->tValue )
        {
            ImGui::SetNextWindowPos( ImVec2( 0.f, 0.f ) );
            if ( ImGui::Begin( "Debug Info", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize ) )
            {
                ImGui::Text( "%.0f FPS (%fms)", 1 / fsecFrameDiff, fsecFrameDiff * 1000.f );
                ImGui::Text( "Frame %i", pRendererState->m_nFramesRendered );
            }
            ImGui::End();
        }

        ImGui::Render();

        WGPUTextureView wgpuNextTexture = wgpuSwapChainGetCurrentTextureView( pRendererState->m_wgpuSwapChain );

        WGPUCommandEncoderDescriptor wgpuCommandEncoderDescriptor {};
        WGPUCommandEncoder wgpuCommandEncoder = wgpuDeviceCreateCommandEncoder( pRendererState->m_wgpuVirtualDevice, &wgpuCommandEncoderDescriptor );
        {
               // describe a render pass
            WGPURenderPassColorAttachment wgpuRenderPassColourAttachment {
                .view = wgpuNextTexture,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue { 0.05, 0.05, 0.05, 1.0 },
            };

            WGPURenderPassDepthStencilAttachment wgpuRenderPassDepthStencilAttachment {
                .view = pRendererState->m_depthTextureAndView.m_wgpuDepthTextureView,
                .depthLoadOp = WGPULoadOp_Clear,
                .depthStoreOp = WGPUStoreOp_Store,
                .depthClearValue = 1.0f,
                .depthReadOnly = false,

                // wgpu and dawn disagree here if stencil isn't in use
                // dawn wants these to be undefined (0) in this case, wgpu will complain if they're ever undefined
#if WEBGPU_BACKEND_WGPU
                .stencilLoadOp = WGPULoadOp_Clear,
                .stencilStoreOp = WGPUStoreOp_Store,
#endif
                .stencilReadOnly = false
            };

            WGPURenderPassDescriptor wgpuRenderPassDescriptor {
                .colorAttachmentCount = 1,
                .colorAttachments = &wgpuRenderPassColourAttachment,
                .depthStencilAttachment = &wgpuRenderPassDepthStencilAttachment
            };

            WGPURenderPassEncoder wgpuRenderPass = wgpuCommandEncoderBeginRenderPass( wgpuCommandEncoder, &wgpuRenderPassDescriptor );
            {
                // Select render pipeline
                // TODO: only support 1 render pipeline/bindgroup atm, should support more at some point!
                wgpuRenderPassEncoderSetPipeline( wgpuRenderPass, pRendererState->m_wgpuObjectRenderPipeline );
                wgpuRenderPassEncoderSetBindGroup( wgpuRenderPass, 0, pRenderView->m_viewUniforms.m_wgpuBindGroup, 0, nullptr );

                for ( int i = 0; i < sRenderableObjects.m_nElements; i++ )
                {
                    // write object state to object uniform if object state has changed
                    RenderableObject *const pRenderableObject = &sRenderableObjects.m_pData[ i ];
                    if ( pRenderableObject->m_bGPUDirty ) // do we need to update the object's state on the gpu?
                    {
                        // TODO: should code be submitting vec3s, or mat4x4s?

                        glm::mat4x4 mat4fTransform = glm::eulerAngleXYZ( pRenderableObject->m_vRotation.x, pRenderableObject->m_vRotation.y, pRenderableObject->m_vRotation.z );
                        mat4fTransform = glm::translate( mat4fTransform, glm::vec3( pRenderableObject->m_vPosition.x, pRenderableObject->m_vPosition.y, pRenderableObject->m_vPosition.z  ) );
                        mat4fTransform = glm::transpose( mat4fTransform );
                        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, sRenderableObjects.m_pData[ i ].m_objectUniforms.m_wgpuBuffer, offsetof( UShaderObjectInstance, m_mat4fBaseTransform ),  &mat4fTransform, sizeof( mat4fTransform ) );

                        pRenderableObject->m_bGPUDirty = false; // gpu has up-to-date state for the object
                    }

                    // set current object and render
                    wgpuRenderPassEncoderSetBindGroup( wgpuRenderPass, 1, sRenderableObjects.m_pData[ i ].m_objectUniforms.m_wgpuBindGroup, 0, nullptr );
                    wgpuRenderPassEncoderSetVertexBuffer( wgpuRenderPass, 0, sRenderableObjects.m_pData[ i ].m_gpuModel.m_wgpuVertexBuffer, 0, sRenderableObjects.m_pData[ i ].m_gpuModel.m_nVertexBufferSize );
                    wgpuRenderPassEncoderSetIndexBuffer( wgpuRenderPass, sRenderableObjects.m_pData[ i ].m_gpuModel.m_wgpuIndexBuffer, WGPUIndexFormat_Uint16, 0, sRenderableObjects.m_pData[ i     ].m_gpuModel.m_nIndexBufferSize );
                    wgpuRenderPassEncoderDrawIndexed( wgpuRenderPass, sRenderableObjects.m_pData[ i ].m_gpuModel.m_nIndexBufferCount, 1, 0, 0, 0 );
                }

                // imgui
                ImGui_ImplWGPU_RenderDrawData( ImGui::GetDrawData(), wgpuRenderPass );
            }
            wgpuRenderPassEncoderEnd( wgpuRenderPass );
            wgpuRenderPassEncoderRelease( wgpuRenderPass );
        }
        WGPUCommandBufferDescriptor wgpuCommandBufferDescriptor {};
        WGPUCommandBuffer wgpuCommand = wgpuCommandEncoderFinish( wgpuCommandEncoder, &wgpuCommandBufferDescriptor );
        wgpuCommandEncoderRelease( wgpuCommandEncoder );

        wgpuTextureViewRelease( wgpuNextTexture );

        wgpuQueueSubmit( pRendererState->m_wgpuQueue, 1, &wgpuCommand );
        wgpuSwapChainPresent( pRendererState->m_wgpuSwapChain ); 

        pRendererState->m_nFramesRendered++;

        fsecFrameDiff = std::chrono::duration<double, std::ratio<1>>( std::chrono::high_resolution_clock::now() - timeBegin ).count();
    }
}
