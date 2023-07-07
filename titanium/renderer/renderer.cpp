#include "renderer_api.hpp"

#include "extern/imgui/imgui.h"
#include "extern/imgui/imgui_impl_wgpu.h"

#include "titanium/util/numerics.hpp"
#include "titanium/sys/platform/sdl/sys_sdl.hpp"
#include "titanium/util/data/staticspan.hpp"
#include "titanium/logger/logger.hpp"
#include "titanium/renderer/utils/stringify.hpp"

#include <chrono> // temp probably, idk if we wanna use this for time
#include <vector>

namespace renderer
{
    void C_WGPUVirtualDeviceHandleError( const WGPUErrorType ewgpuErrorType, const char * const pszMessage, void *const pUserdata )
    {
        logger::Info( "%s: type: %s, %s" ENDL, __FUNCTION__, renderer::util::wgpu::ErrorTypeToString( ewgpuErrorType ), pszMessage );
    }

    WGPUSwapChain WGPUVirtualDevice_CreateSwapChainForWindow( WGPUDevice wgpuVirtualDevice, SDL_Window *const psdlWindow, WGPUSurface wgpuSurface, WGPUAdapter wgpuAdapter )
    {
        int nWindowWidth, nWindowHeight;
        SDL_GetWindowSize( psdlWindow, &nWindowWidth, &nWindowHeight );
        
        // get supported surface present modes, ideally take the preferred one, but if that's not available, get the default supported
        WGPUPresentMode wgpuPreferredPresentMode = WGPUPresentMode_Immediate; // TODO: make configurable
        const WGPUPresentMode wgpuPresentMode = [ wgpuSurface, wgpuAdapter, wgpuPreferredPresentMode ](){
            // wgpuSurfaceGetCapabilities function populates user-provided pointers in the struct passed to it
            // so we need to provide a buffer for all the array fields we want to query before calling (in this case, presentModes)
            // TODO: remove magic number, this is the number of entries in the WGPUPresentMode enum

            // TODO: nonstandard
            /*/::util::data::StaticSpan<WGPUPresentMode, 4> sPresetModes;
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
            }*/

            // i *believe* it is required for fifo to be supported (this is the case in vulkan) so default to it
            // TODO reexamine this, unsure if it's right
            return WGPUPresentMode_Fifo; 
        }();

        WGPUTextureFormat wgpuSwapchainFormat;
#if WEBGPU_BACKEND_WGPU 
        wgpuSwapchainFormat = wgpuSurfaceGetPreferredFormat( wgpuSurface, wgpuAdapter );
#elif WEBGPU_BACKEND_DAWN
        wgpuSwapchainFormat = WGPUTextureFormat_BGRA8Unorm; // dawn only supports this, and doesn't have wgpuSurfaceGetPreferredFormat
#endif 

        const WGPUSwapChainDescriptor wgpuSwapChainDescriptor {
            .usage = WGPUTextureUsage_RenderAttachment,
            .format = wgpuSwapchainFormat,

            .width = static_cast<u32>( nWindowWidth ),
            .height = static_cast<u32>( nWindowHeight ),

            .presentMode = wgpuPresentMode 
        };

        // TODO: this is a bit spammy, add when we have designated spam logs
        //logger::Info( "Creating swapchain: width %i, height %i present mode: %s" ENDL, nWindowWidth, nWindowHeight, renderer::util::wgpu::PresentModeToString( wgpuPresentMode ) );
        return wgpuDeviceCreateSwapChain( wgpuVirtualDevice, wgpuSurface, &wgpuSwapChainDescriptor );
    }

    WGPUTextureView WGPUVirtualDevice_CreateDepthTextureAndViewForWindow( WGPUDevice wgpuVirtualDevice, SDL_Window *const psdlWindow )
    {
        int nWindowWidth, nWindowHeight;
        SDL_GetWindowSize( psdlWindow, &nWindowWidth, &nWindowHeight );

        // make depth buffer
        // TODO: const or unify between depthstencilstate and here somehow
        //
        static const WGPUTextureFormat wgpuDepthTextureFormat = WGPUTextureFormat_Depth24Plus; 

        WGPUTextureDescriptor wgpuDepthTextureDescriptor {
            .usage = WGPUTextureUsage_RenderAttachment,
            .dimension = WGPUTextureDimension_2D,
            .size = { static_cast<u32>( nWindowWidth ), static_cast<u32>( nWindowHeight ), 1 }, // TODO: TEMP!!!!! should recreate on preframe::ResolutionChanged
            .format = WGPUTextureFormat_Depth24Plus, 
            .mipLevelCount = 1,
            .sampleCount = 1,
            .viewFormatCount = 1,
            .viewFormats = &wgpuDepthTextureFormat
        };

        // TODO: we probably aren't cleaning this up...
        WGPUTexture wgpuDepthTexture = wgpuDeviceCreateTexture( wgpuVirtualDevice, &wgpuDepthTextureDescriptor );
    
        WGPUTextureViewDescriptor wgpuDepthTextureViewDescriptor {
            .format = WGPUTextureFormat_Depth24Plus,
            .dimension = WGPUTextureViewDimension_2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
            .aspect = WGPUTextureAspect_All
        };

        return wgpuTextureCreateView( wgpuDepthTexture, &wgpuDepthTextureViewDescriptor );
    }

	void InitialisePhysicalRenderingDevice( TitaniumPhysicalRenderingDevice *const pRendererDevice )
    {
        logger::Info( "Initialising wgpu rendering device..." ENDL );
        //logger::Info( "wgpu version is %i" ENDL, wgpuGetVersion() ); // nonstandard :c

        const WGPUInstanceDescriptor wgpuDesc {};
        WGPUInstance wgpuInstance = pRendererDevice->m_wgpuInstance = wgpuCreateInstance( &wgpuDesc );

        // Request graphics adapter from instance
        WGPUAdapter wgpuAdapter = pRendererDevice->m_wgpuAdapter = [ wgpuInstance ]()
        {
            const WGPURequestAdapterOptions wgpuAdapterOptions { 
                // note: .compatibleSurface could be used here, but we don't have a surface at this point, fortunately it isn't required
                .powerPreference = WGPUPowerPreference_HighPerformance,
                .forceFallbackAdapter = false // TODO: make this configurable through config var
            };

			WGPUAdapter r_wgpuAdapter;
            wgpuInstanceRequestAdapter( 
                wgpuInstance, 
                &wgpuAdapterOptions,  
                []( const WGPURequestAdapterStatus wgpuRequestAdapterStatus, WGPUAdapter wgpuAdapter, const char *const pszMessage, void *const pUserdata ) 
                {
                    logger::Info( "wgpuInstanceRequestAdapter returned %s, with message: %s" ENDL, renderer::util::wgpu::RequestAdapterStatusToString( wgpuRequestAdapterStatus ), pszMessage );
                    
                    if ( wgpuRequestAdapterStatus == WGPURequestAdapterStatus_Success )
                    {
                       *reinterpret_cast<WGPUAdapter *>( pUserdata ) = wgpuAdapter;
                    }
               },
               &r_wgpuAdapter
            );

            return r_wgpuAdapter;
        }();

        // Print info about the graphics adapter
        [ wgpuAdapter ]()
        {
            WGPUAdapterProperties wgpuAdapterProperties {};
            wgpuAdapterGetProperties( wgpuAdapter, &wgpuAdapterProperties );

            logger::Info( 
                "Found Adapter: %s %s, using driver %s on %s" ENDL, 
                renderer::util::wgpu::AdapterTypeToString( wgpuAdapterProperties.adapterType ), 
                wgpuAdapterProperties.name,

                wgpuAdapterProperties.driverDescription,

                renderer::util::wgpu::BackendTypeToString( wgpuAdapterProperties.backendType ) 
            );

            WGPUSupportedLimits wgpuAdapterLimits;
            wgpuAdapterGetLimits( wgpuAdapter, &wgpuAdapterLimits );

            logger::Info( "Adapter Limits:" ENDL
                          "\tmaxTextureDimension1D = %u" ENDL
                          "\tmaxTextureDimension2D = %u" ENDL
                          "\tmaxTextureDimension3D = %u" ENDL
                          "\tmaxTextureArrayLayers = %u" ENDL
                          "\tmaxBindGroups = %u" ENDL
                          "\tmaxBindingsPerBindGroup = %u" ENDL
                          "\tmaxDynamicUniformBuffersPerPipelineLayout = %u" ENDL
                          "\tmaxDynamicStorageBuffersPerPipelineLayout = %u" ENDL
                          "\tmaxSampledTexturesPerShaderStage = %u" ENDL
                          "\tmaxSamplersPerShaderStage = %u" ENDL
                          "\tmaxStorageBuffersPerShaderStage = %u" ENDL
                          "\tmaxStorageTexturesPerShaderStage = %u" ENDL
                          "\tmaxUniformBuffersPerShaderStage = %u" ENDL
                          "\tmaxUniformBufferBindingSize = %llu" ENDL
                          "\tmaxStorageBufferBindingSize = %llu" ENDL
                          "\tminUniformBufferOffsetAlignment = %u" ENDL
                          "\tminStorageBufferOffsetAlignment = %u" ENDL
                          "\tmaxVertexBuffers = %u" ENDL
                          "\tmaxBufferSize = %llu" ENDL
                          "\tmaxVertexAttributes = %u" ENDL
                          "\tmaxVertexBufferArrayStride = %u" ENDL
                          "\tmaxInterStageShaderComponents = %u" ENDL
                          "\tmaxInterStageShaderVariables = %u" ENDL
                          "\tmaxColorAttachments = %u" ENDL
                          "\tmaxColorAttachmentBytesPerSample = %u" ENDL
                          "\tmaxComputeWorkgroupStorageSize = %u" ENDL
                          "\tmaxComputeInvocationsPerWorkgroup = %u" ENDL
                          "\tmaxComputeWorkgroupSizeX = %u" ENDL
                          "\tmaxComputeWorkgroupSizeY = %u" ENDL
                          "\tmaxComputeWorkgroupSizeZ = %u" ENDL
                          "\tmaxComputeWorkgroupsPerDimension = %u" ENDL,
                          wgpuAdapterLimits.limits.maxTextureDimension1D,
                          wgpuAdapterLimits.limits.maxTextureDimension2D,
                          wgpuAdapterLimits.limits.maxTextureDimension3D,
                          wgpuAdapterLimits.limits.maxTextureArrayLayers,
                          wgpuAdapterLimits.limits.maxBindGroups,
                          wgpuAdapterLimits.limits.maxBindingsPerBindGroup,
                          wgpuAdapterLimits.limits.maxDynamicUniformBuffersPerPipelineLayout,
                          wgpuAdapterLimits.limits.maxDynamicStorageBuffersPerPipelineLayout,
                          wgpuAdapterLimits.limits.maxSampledTexturesPerShaderStage,
                          wgpuAdapterLimits.limits.maxSamplersPerShaderStage,
                          wgpuAdapterLimits.limits.maxStorageBuffersPerShaderStage,
                          wgpuAdapterLimits.limits.maxStorageTexturesPerShaderStage,
                          wgpuAdapterLimits.limits.maxUniformBuffersPerShaderStage,
                          wgpuAdapterLimits.limits.maxUniformBufferBindingSize,
                          wgpuAdapterLimits.limits.maxStorageBufferBindingSize,
                          wgpuAdapterLimits.limits.minUniformBufferOffsetAlignment,
                          wgpuAdapterLimits.limits.minStorageBufferOffsetAlignment,
                          wgpuAdapterLimits.limits.maxVertexBuffers,
                          wgpuAdapterLimits.limits.maxBufferSize,
                          wgpuAdapterLimits.limits.maxVertexAttributes,
                          wgpuAdapterLimits.limits.maxVertexBufferArrayStride,
                          wgpuAdapterLimits.limits.maxInterStageShaderComponents,
                          wgpuAdapterLimits.limits.maxInterStageShaderVariables,
                          wgpuAdapterLimits.limits.maxColorAttachments,
                          wgpuAdapterLimits.limits.maxColorAttachmentBytesPerSample,
                          wgpuAdapterLimits.limits.maxComputeWorkgroupStorageSize,
                          wgpuAdapterLimits.limits.maxComputeInvocationsPerWorkgroup,
                          wgpuAdapterLimits.limits.maxComputeWorkgroupSizeX,
                          wgpuAdapterLimits.limits.maxComputeWorkgroupSizeY,
                          wgpuAdapterLimits.limits.maxComputeWorkgroupSizeZ,
                          wgpuAdapterLimits.limits.maxComputeWorkgroupsPerDimension );

            ::util::data::StaticSpan<WGPUFeatureName, 32> swgpuFeatures;
            size_t nFeatures = wgpuAdapterEnumerateFeatures( wgpuAdapter, swgpuFeatures.m_tData );

            if ( nFeatures )
            {
                logger::Info( "Adapter has features: " ENDL );

                for ( int i = 0; i < nFeatures; i++ )
                {
                    // TODO: track down why we're getting invalid enum values in this
                    logger::Info( "\t%s" ENDL, renderer::util::wgpu::FeatureNameToString( swgpuFeatures.m_tData[ i ] ) );
                } 
            }
        }();
    }

    void Initialise( TitaniumPhysicalRenderingDevice *const pRendererDevice, TitaniumRendererState *const pRendererState, SDL_Window *const psdlWindow )
    {
        logger::Info( "Initialising wgpu renderer..." ENDL );

        WGPUInstance wgpuInstance = pRendererDevice->m_wgpuInstance;
        WGPUAdapter wgpuAdapter = pRendererDevice->m_wgpuAdapter;

        pRendererState->m_psdlWindow = psdlWindow;
        WGPUSurface wgpuSurface = pRendererState->m_wgpuSurface = sys::platform::sdl::CreateWGPUSurfaceForWindow( psdlWindow, wgpuInstance );

        WGPUTextureFormat wgpuSwapchainFormat;
#if WEBGPU_BACKEND_WGPU 
        wgpuSwapchainFormat = wgpuSurfaceGetPreferredFormat( wgpuSurface, wgpuAdapter );
#elif WEBGPU_BACKEND_DAWN
        wgpuSwapchainFormat = WGPUTextureFormat_BGRA8Unorm; // dawn only supports this, and doesn't have wgpuSurfaceGetPreferredFormat
#endif 

        // Request virtual graphics device from physical adapter
        WGPUDevice wgpuVirtualDevice = pRendererState->m_wgpuVirtualDevice = [ wgpuAdapter ]()
        {
            const WGPUDeviceDescriptor wgpuVirtualDeviceDescriptor { };
            WGPUDevice r_wgpuVirtualDevice;
            wgpuAdapterRequestDevice(
                wgpuAdapter,
                &wgpuVirtualDeviceDescriptor,
                []( const WGPURequestDeviceStatus wgpuRequestDeviceStatus, WGPUDevice wgpuVirtualDevice, const char *const pszMessage, void *const pUserdata )
                {
                    logger::Info( "wgpuRequestDeviceStatus returned %s, with message: %s" ENDL, renderer::util::wgpu::RequestDeviceStatusToString( wgpuRequestDeviceStatus ), pszMessage );
                    
                    if ( wgpuRequestDeviceStatus == WGPURequestDeviceStatus_Success )
                    {
                        *reinterpret_cast<WGPUDevice *>( pUserdata ) = wgpuVirtualDevice;
                    }
                },
                &r_wgpuVirtualDevice
            );

            return r_wgpuVirtualDevice;
        }();

        wgpuDeviceSetUncapturedErrorCallback( wgpuVirtualDevice, C_WGPUVirtualDeviceHandleError, &wgpuVirtualDevice );

        // Request queue from virtual device
        WGPUQueue wgpuQueue = pRendererState->m_wgpuQueue = wgpuDeviceGetQueue( wgpuVirtualDevice );
       
        // Request swapchain from device
        pRendererState->m_wgpuSwapChain = WGPUVirtualDevice_CreateSwapChainForWindow( wgpuVirtualDevice, psdlWindow, wgpuSurface, wgpuAdapter );

        // Request uniform buffer from device
        WGPUBuffer wgpuUniformBuffer = pRendererState->m_wgpuUniformBuffer = [ wgpuVirtualDevice, wgpuQueue, psdlWindow ](){ 
            WGPUBufferDescriptor wgpuUniformBufferDescriptor {
                .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
                .size = sizeof( f32 ) + sizeof( u32 ) * 2
            };

            WGPUBuffer wgpuUniformBuffer = wgpuDeviceCreateBuffer( wgpuVirtualDevice, &wgpuUniformBufferDescriptor );

            const float flTimeBegin = 0.0f;
            wgpuQueueWriteBuffer( wgpuQueue, wgpuUniformBuffer, 0, &flTimeBegin, sizeof( f32 ) );
            int nWindowWidth, nWindowHeight;
            SDL_GetWindowSize( psdlWindow, &nWindowWidth, &nWindowHeight );
            wgpuQueueWriteBuffer( wgpuQueue, wgpuUniformBuffer, sizeof( f32 ), &nWindowWidth, sizeof( u32 ) );
            wgpuQueueWriteBuffer( wgpuQueue, wgpuUniformBuffer, sizeof( f32 ) + sizeof( u32 ), &nWindowHeight, sizeof( u32 ) );

            return wgpuUniformBuffer;
        }();

        // Make uniform bind group layout and uniform bind group
        // TODO: could we macro this syntax maybe?
        struct R_MakeUniformBindGroup { WGPUBindGroupLayout wgpuBindGroupLayout; WGPUBindGroup wgpuBindGroup; };
        auto [ wgpuBindGroupLayout, wgpuBindGroup ] = [ wgpuVirtualDevice, wgpuUniformBuffer ](){            
            WGPUBindGroupLayoutEntry wgpuBindingLayout {
                .binding = 0,
                .visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Vertex,

                .buffer {
                    .type = WGPUBufferBindingType_Uniform,
                    .minBindingSize = sizeof( f32 ) + sizeof( u32 ) * 2
                }
            };

            WGPUBindGroupLayoutDescriptor wgpuBindGroupLayoutDescriptor {
                .entryCount = 1,
                .entries = &wgpuBindingLayout
            };

            WGPUBindGroupLayout r_wgpuBindGroupLayout = wgpuDeviceCreateBindGroupLayout( wgpuVirtualDevice, &wgpuBindGroupLayoutDescriptor );

            WGPUBindGroupEntry wgpuBinding {
                .binding = 0,
                .buffer = wgpuUniformBuffer,
                .offset = 0,
                .size = sizeof( f32 ) + sizeof( u32 ) * 2
            };

            WGPUBindGroupDescriptor wgpuBindGroupDescriptor {
                .layout = r_wgpuBindGroupLayout,
                .entryCount = 1, // keep same as wgpuBindGroupLayoutDescriptor.entryCount!
                .entries = &wgpuBinding
            };

            WGPUBindGroup r_wgpuBindGroup = wgpuDeviceCreateBindGroup( wgpuVirtualDevice, &wgpuBindGroupDescriptor );

            return R_MakeUniformBindGroup { r_wgpuBindGroupLayout, r_wgpuBindGroup };
        }();

        pRendererState->m_wgpuUniformBindGroup = wgpuBindGroup;

        // Request pipeline layout from device
        WGPUPipelineLayout wgpuPipelineLayout = [ wgpuVirtualDevice, wgpuBindGroupLayout ](){
            WGPUPipelineLayoutDescriptor wgpuPipelineLayoutDescriptor {
                .bindGroupLayoutCount = 1,
                .bindGroupLayouts = &wgpuBindGroupLayout
            };

            return wgpuDeviceCreatePipelineLayout( wgpuVirtualDevice, &wgpuPipelineLayoutDescriptor );
        }();

        // Create render pipeline for device
        pRendererState->m_wgpuRenderPipeline = [ wgpuVirtualDevice, wgpuSwapchainFormat, wgpuPipelineLayout ]()
        {
            // Create shader module
            WGPUShaderModule wgpuShaderModule = [ wgpuVirtualDevice ]()
            {
                WGPUShaderModuleWGSLDescriptor wgpuShaderCodeDescriptor {
                    .chain { .sType = WGPUSType_ShaderModuleWGSLDescriptor },
                    .code = R"(
                                struct U_Input 
                                {
                                    flTime : f32,
                                    nWindowHeight : u32,
                                    nWindowWidth : u32
                                };

                                @group( 0 ) @binding( 0 ) var<uniform> u_input : U_Input;

                                struct VertexInput
                                {
                                    @location( 0 ) position : vec3<f32>,
                                    @location( 1 ) colour : vec3<f32>
                                };

                                struct R_VertexOutput
                                {
                                    @builtin( position ) position : vec4<f32>,
                                    @location( 0 ) colour : vec3<f32>
                                };

                                @vertex fn vs_main( in : VertexInput ) -> R_VertexOutput
                                {
                                    var r_out : R_VertexOutput;

                                    let flObjectAngle = u_input.flTime;
                                    let flObjectAngleC = cos( flObjectAngle );
                                    let flObjectAngleS = sin( flObjectAngle );

                                    let flViewAngle = 3.0 * 3.14159 / 4.0; // three 8th of turn (1 turn = 2 pi)
                                    let flViewAngleC = cos( flViewAngle );
                                    let flViewAngleS = sin( flViewAngle );

                                    let MCompose = 
                                    // rotate the viewpoint in the YZ plane
                                    transpose( mat4x4<f32> ( 
                                        1.0, 0.0,           0.0,            0.0,
                                        0.0, flViewAngleC,  flViewAngleS,   0.0,
                                        0.0, -flViewAngleS, flViewAngleC,   0.0,
                                        0.0, 0.0,           0.0,            1.0
                                    ) ) *
                                    
                                    // rotate the model in the XY plane
                                    transpose( mat4x4<f32>( 
                                        flObjectAngleC,  flObjectAngleS, 0.0, 0.0,
                                        -flObjectAngleS, flObjectAngleC, 0.0, 0.0,
                                        0.0,             0.0,            1.0, 0.0,
                                        0.0,             0.0,            0.0, 1.0
                                    ) ) *

                                    // translate it at an offset from its current direction
                                    transpose( mat4x4<f32>( 
                                        0.3, 0.0, 0.0, 0.5,
                                        0.0, 0.3, 0.0, 0.0,
                                        0.0, 0.0, 0.3, 0.0,
                                        0.0, 0.0, 0.0, 1.0
                                    ) );

                                    let near = -1.0;
                                    let far = 1.0;
                                    let scale = 1.0;
                                    let ratio = f32( u_input.nWindowHeight ) / f32( u_input.nWindowWidth );
                                    let MProjectOrthographic = transpose( mat4x4<f32> (
                                        1.0 / scale, 0.0,           0.0,                0.0,
                                        0.0,         ratio / scale, 0.0,                0.0,
                                        0.0,         0.0,           1.0 / (far - near), -near / (far - near),
                                        0.0,         0.0,           0.0,                1.0,
                                    ) );


                                    let finalPosition = MProjectOrthographic * MCompose * vec4<f32>( in.position, 1.0 );

                                    r_out.position = finalPosition;
                                    r_out.colour = r_out.position.xyz;

                                    return r_out;
                                }

                                @fragment fn fs_main( in : R_VertexOutput ) -> @location( 0 ) vec4<f32> 
                                {
                                    return vec4<f32>( pow( in.colour + ( sin( u_input.flTime ) * 0.4 ), vec3<f32>( 2.2 ) ), 1.0 );
                                }
                            )"
                };

                WGPUShaderModuleDescriptor wgpuShaderDescriptor {
                    .nextInChain = &wgpuShaderCodeDescriptor.chain
                };

                return wgpuDeviceCreateShaderModule( wgpuVirtualDevice, &wgpuShaderDescriptor );
            }();

            ::util::data::StaticSpan<WGPUVertexAttribute, 2> sVertexAttributes {
                // position attribute
                {
                    .format = WGPUVertexFormat_Float32x3,
                    .offset = 0,
                    .shaderLocation = 0
                },

                // colour attribute
                {
                    .format = WGPUVertexFormat_Float32x3,
                    .offset = 3 * sizeof( float ),
                    .shaderLocation = 1
                }
            };

            WGPUVertexBufferLayout wgpuVertexBufferLayout {
                .arrayStride = sizeof( float ) * 6,
                .stepMode = WGPUVertexStepMode_Vertex,
                .attributeCount = static_cast<u32>( sVertexAttributes.Elements() ),
                .attributes = sVertexAttributes.m_tData
            };

            WGPUDepthStencilState wgpuDepthStencilState {
                .format = WGPUTextureFormat_Depth24Plus,
                .depthWriteEnabled = true,
                .depthCompare = WGPUCompareFunction_Less,

                .stencilFront = { .compare = WGPUCompareFunction_Always },
                .stencilBack = { .compare = WGPUCompareFunction_Always },
                .stencilReadMask = 0xFFFFFFFF,
                .stencilWriteMask = 0xFFFFFFFF
            };

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
                    .stripIndexFormat = WGPUIndexFormat_Undefined,
                    .frontFace = WGPUFrontFace_CCW,
                    .cullMode = WGPUCullMode_None //WGPUCullMode_Back
                },

                .depthStencil = &wgpuDepthStencilState,

                .multisample {
                    .count = 1, // disable multisampling
                    .mask = ~0u,
                    .alphaToCoverageEnabled = false
                },

                .fragment = &wgpuFragmentState // this sucks, wish there was a way of aggregate-initialising this inline
            };

            return wgpuDeviceCreateRenderPipeline( wgpuVirtualDevice, &wgpuRenderPipelineDescriptor );
        }();

        // make mesh buffers
        struct R_MakeMeshBuffers { 
            WGPUBuffer wgpuVertexBuffer; size_t nVertexBufferSize;
            WGPUBuffer wgpuIndexBuffer; size_t nIndexBufferSize; int nIndexBufferCount;
        };
        auto [ wgpuVertexBuffer, nVertexBufferSize, wgpuIndexBuffer, nIndexBufferSize, nIndexBufferCount ] = [ wgpuVirtualDevice, wgpuQueue ](){
            // create vertex buffer
            ::util::data::StaticSpan<float, 30> sflTempVertexData {
              // x,    y,    z,      r,   g,   b
                // The base
                -0.5, -0.5, -0.3,    1.0, 1.0, 1.0,
                +0.5, -0.5, -0.3,    1.0, 1.0, 1.0,
                +0.5, +0.5, -0.3,    1.0, 1.0, 1.0,
                -0.5, +0.5, -0.3,    1.0, 1.0, 1.0,

                // And the tip of the pyramid!
                +0.0, +0.0, +0.5,    0.5, 0.5, 0.5
            };

            ::util::data::StaticSpan<u16, 18> snTempIndexData {
                // Base
                0, 1, 2,
                0, 2, 3,

                // Sides
                0, 1, 4,
                1, 2, 4,
                2, 3, 4,
                3, 0, 4
            };
            
            WGPUBufferDescriptor wgpuVertexBufferDescriptor {
                .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
                .size = sflTempVertexData.Size(),
            };

            WGPUBuffer wgpuVertexBuffer = wgpuDeviceCreateBuffer( wgpuVirtualDevice, &wgpuVertexBufferDescriptor );
            wgpuQueueWriteBuffer( wgpuQueue, wgpuVertexBuffer, 0, sflTempVertexData.m_tData, sflTempVertexData.Size() );

            WGPUBufferDescriptor wgpuIndexBufferDescriptor {
                .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
                .size = snTempIndexData.Size() 
            };

            WGPUBuffer wgpuIndexBuffer = wgpuDeviceCreateBuffer( wgpuVirtualDevice, &wgpuIndexBufferDescriptor );
            wgpuQueueWriteBuffer( wgpuQueue, wgpuIndexBuffer, 0, snTempIndexData.m_tData, snTempIndexData.Size() );

            return R_MakeMeshBuffers { 
                wgpuVertexBuffer, sflTempVertexData.Size(),
                wgpuIndexBuffer, snTempIndexData.Size(), static_cast<int>( snTempIndexData.Elements() )
            };
        }();

        pRendererState->m_wgpuVertexBuffer = wgpuVertexBuffer;
        pRendererState->m_nVertexBufferSize = nVertexBufferSize;
        pRendererState->m_wgpuIndexBuffer = wgpuIndexBuffer;
        pRendererState->m_nIndexBufferSize = nIndexBufferSize;
        pRendererState->m_nIndexBufferCount = nIndexBufferCount;

        pRendererState->m_wgpuDepthTextureView = WGPUVirtualDevice_CreateDepthTextureAndViewForWindow( wgpuVirtualDevice, psdlWindow );

        // imgui init
        // TODO: what's the significance of having multiple frames in flight??? is this important??? idk doing 1
        ImGui_ImplWGPU_Init( wgpuVirtualDevice, 1, wgpuSwapchainFormat, WGPUTextureFormat_Depth24Plus );

        logger::Info( "wgpu renderer initialised successfully!" ENDL );
    }

    void preframe::ResolutionChanged( TitaniumPhysicalRenderingDevice *const pRendererDevice, TitaniumRendererState *const pRendererState )
    {
        // swapchains rely on the window's resolution, so need to be recreated on window resize
        wgpuSwapChainRelease( pRendererState->m_wgpuSwapChain ); // destroy old swapchain
        pRendererState->m_wgpuSwapChain = WGPUVirtualDevice_CreateSwapChainForWindow( 
            pRendererState->m_wgpuVirtualDevice, 
            pRendererState->m_psdlWindow, 
            pRendererState->m_wgpuSurface, 
            pRendererDevice->m_wgpuAdapter
        );

        pRendererState->m_wgpuDepthTextureView = WGPUVirtualDevice_CreateDepthTextureAndViewForWindow( pRendererState->m_wgpuVirtualDevice, pRendererState->m_psdlWindow );

        // TODO: TEMP!!! SUCKS!!!
        int nWindowWidth, nWindowHeight;
        SDL_GetWindowSize( pRendererState->m_psdlWindow, &nWindowWidth, &nWindowHeight );
        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, pRendererState->m_wgpuUniformBuffer, sizeof( f32 ), &nWindowWidth, sizeof( u32 ) );
        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, pRendererState->m_wgpuUniformBuffer, sizeof( f32 ) + sizeof( u32 ), &nWindowHeight, sizeof( u32 ) );
    }

    void preframe::ImGUI( TitaniumRendererState *const pRendererState )
    {
        ImGui_ImplWGPU_NewFrame();
    }

    void Frame( TitaniumRendererState *const pRendererState )
    {
#if WEBGPU_BACKEND_WGPU 
        // TODO: what do i actually need here??
        //wgpuDevicePoll( pRendererState->m_wgpuVirtualDevice );
#elif WEBGPU_BACKEND_DAWN
        wgpuDeviceTick( pRendererState->m_wgpuVirtualDevice );
#endif 

        static auto programTimeBegin = std::chrono::high_resolution_clock::now();
        static float fsecFrameDiff = 0.0f;

        auto timeBegin = std::chrono::high_resolution_clock::now();

        float flCurrentTime = std::chrono::duration<double, std::ratio<1>>( std::chrono::high_resolution_clock::now() - programTimeBegin ).count();
        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, pRendererState->m_wgpuUniformBuffer, 0, &flCurrentTime, sizeof( float ) );

        ImGui::SetNextWindowPos( ImVec2( 0.f, 0.f ) );
        if ( ImGui::Begin( "Debug Info", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize ) )
        {
            ImGui::Text( "%.0f FPS", 1 / fsecFrameDiff );
            ImGui::Text( "Frame %i", pRendererState->m_nFramesRendered );
        }
        ImGui::End();

        ImGui::Render();

        WGPUTextureView wgpuNextTexture = wgpuSwapChainGetCurrentTextureView( pRendererState->m_wgpuSwapChain );

        WGPUCommandEncoderDescriptor wgpuCommandEncoderDescriptor {};
        WGPUCommandEncoder wgpuCommandEncoder = wgpuDeviceCreateCommandEncoder( pRendererState->m_wgpuVirtualDevice, &wgpuCommandEncoderDescriptor );

        // describe a render pass
        WGPURenderPassColorAttachment wgpuRenderPassColourAttachment {
            .view = wgpuNextTexture,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue { 0.05, 0.05, 0.05, 1.0 },
        };

        WGPURenderPassDepthStencilAttachment wgpuRenderPassDepthStencilAttachment {
            .view = pRendererState->m_wgpuDepthTextureView,
            .depthLoadOp = WGPULoadOp_Clear,
            .depthStoreOp = WGPUStoreOp_Store,
            .depthClearValue = 1.0f,
            .depthReadOnly = false,

		    .stencilLoadOp = WGPULoadOp_Clear,
		    .stencilStoreOp = WGPUStoreOp_Store,
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
            wgpuRenderPassEncoderSetPipeline( wgpuRenderPass, pRendererState->m_wgpuRenderPipeline );
            wgpuRenderPassEncoderSetBindGroup( wgpuRenderPass, 0, pRendererState->m_wgpuUniformBindGroup, 0, nullptr );
            wgpuRenderPassEncoderSetVertexBuffer( wgpuRenderPass, 0, pRendererState->m_wgpuVertexBuffer, 0, pRendererState->m_nVertexBufferSize );
            wgpuRenderPassEncoderSetIndexBuffer( wgpuRenderPass, pRendererState->m_wgpuIndexBuffer, WGPUIndexFormat_Uint16, 0, pRendererState->m_nIndexBufferSize );
            wgpuRenderPassEncoderDrawIndexed( wgpuRenderPass, pRendererState->m_nIndexBufferCount, 1, 0, 0, 0 );

            ImGui_ImplWGPU_RenderDrawData( ImGui::GetDrawData(), wgpuRenderPass );
        }
        wgpuRenderPassEncoderEnd( wgpuRenderPass );
        wgpuRenderPassEncoderRelease( wgpuRenderPass );

        wgpuTextureViewRelease( wgpuNextTexture );

        WGPUCommandBufferDescriptor wgpuCommandBufferDescriptor {};
        WGPUCommandBuffer wgpuCommand = wgpuCommandEncoderFinish( wgpuCommandEncoder, &wgpuCommandBufferDescriptor );
        wgpuCommandEncoderRelease( wgpuCommandEncoder );

        wgpuQueueSubmit( pRendererState->m_wgpuQueue, 1, &wgpuCommand );
        wgpuSwapChainPresent( pRendererState->m_wgpuSwapChain ); 

        pRendererState->m_nFramesRendered++;

        fsecFrameDiff = std::chrono::duration<double, std::ratio<1>>( std::chrono::high_resolution_clock::now() - timeBegin ).count();
    }
}
