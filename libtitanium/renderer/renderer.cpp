#include "renderer.hpp"

#include "extern/imgui/imgui.h"
#include "extern/imgui/imgui_impl_wgpu.h"

#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <libtitanium/util/numerics.hpp>
#include <libtitanium/util/maths.hpp>
#include <libtitanium/sys/platform_sdl.hpp>
#include <libtitanium/util/data/staticspan.hpp>
#include <libtitanium/logger/logger.hpp>
#include <libtitanium/config/config.hpp>
#include <libtitanium/renderer/stringify.hpp>

#include <chrono> // temp probably, idk if we wanna use this for time
#include <webgpu/webgpu.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/euler_angles.hpp>


// TODO: the renderer is super complex and will only get more complex, it should really be split into multiple files
// atm, code is a bit unwieldly, hard to navigate and understand at a glance which sucks

namespace renderer
{
    config::Var<bool> * g_pbcvarPreferLowPowerAdapter = config::RegisterVar<bool>( "renderer:device:preferlowpoweradapter", false, config::EFVarUsageFlags::STARTUP );
    config::Var<bool> * g_pbcvarForceFallbackAdapter = config::RegisterVar<bool>( "renderer:device:forcefallbackadapter", false, config::EFVarUsageFlags::STARTUP );

    config::Var<bool> * g_pbcvarPreferImmediatePresent = config::RegisterVar<bool>( "renderer:preferimmediatepresent", false, config::EFVarUsageFlags::NONE );

    config::Var<bool> * g_pbcvarShowFps = config::RegisterVar<bool>( "renderer:showfps", false, config::EFVarUsageFlags::NONE );

    // TODO: temp structs
    #pragma pack( push, 16 )
    struct UShaderGlobals
    {
        glm::mat4x4 m_mat4fCameraTransform;
        float m_flTime;
        u32 pad;
        ::util::maths::Vec2<u32> m_vWindowSize;
    };
    #pragma pack( pop, 16 )
    static_assert( sizeof( UShaderGlobals ) % 16 == 0 );

    #pragma pack( push, 16 )
    struct UShaderStandardVars
    {
        glm::mat4x4 m_mat4fBaseTransform;
    };
    #pragma pack( pop, 16 )
    static_assert( sizeof( UShaderStandardVars ) % 16 == 0 );

	void InitialisePhysicalRenderingDevice( TitaniumPhysicalRenderingDevice *const pRendererDevice )
    {
        logger::Info( "Initialising wgpu rendering device..." ENDL );
        //logger::Info( "wgpu version is %i" ENDL, wgpuGetVersion() ); // nonstandard :<

        const WGPUInstanceDescriptor wgpuInstanceDescriptor {};
        WGPUInstance wgpuInstance = pRendererDevice->m_wgpuInstance = wgpuCreateInstance( &wgpuInstanceDescriptor );

        // Request graphics adapter from instance
        WGPUAdapter wgpuAdapter = pRendererDevice->m_wgpuAdapter = [ wgpuInstance ]()
        {
            const WGPURequestAdapterOptions wgpuAdapterOptions { 
                // note: .compatibleSurface could be used here, but we don't have a surface at this point, fortunately it isn't required
                .powerPreference = g_pbcvarPreferLowPowerAdapter->tValue ? WGPUPowerPreference_LowPower : WGPUPowerPreference_HighPerformance,
                .forceFallbackAdapter = g_pbcvarForceFallbackAdapter->tValue
            };

            const char * pszLogAdapter = "unknown";
            if ( wgpuAdapterOptions.forceFallbackAdapter )
            {
                pszLogAdapter = "fallback";
            }
            else
            {
                if ( wgpuAdapterOptions.powerPreference == WGPUPowerPreference_LowPower )
                {
                    pszLogAdapter = "low power";
                }
                else if ( wgpuAdapterOptions.powerPreference == WGPUPowerPreference_HighPerformance )
                {
                    pszLogAdapter = "high performance";
                }
            }

            logger::Info( "Requesting %s adapter..." ENDL, pszLogAdapter );

			WGPUAdapter r_wgpuAdapter;
            wgpuInstanceRequestAdapter( 
                wgpuInstance, 
                &wgpuAdapterOptions,  
                []( const WGPURequestAdapterStatus wgpuRequestAdapterStatus, WGPUAdapter wgpuAdapter, const char *const pszMessage, void *const pUserdata ) 
                {
                    logger::Info( "wgpuInstanceRequestAdapter returned %s, with message: %s" ENDL, WGPURequestAdapterStatusToString( wgpuRequestAdapterStatus ), pszMessage );
                    
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
                WGPUAdapterTypeToString( wgpuAdapterProperties.adapterType ),
                wgpuAdapterProperties.name,

                wgpuAdapterProperties.driverDescription,

                WGPUBackendTypeToString( wgpuAdapterProperties.backendType )
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
                    logger::Info( "\t%s" ENDL, WGPUFeatureNameToString( swgpuFeatures.m_tData[ i ] ) );
                } 
            }
        }();
    }
    
    void C_WGPUVirtualDeviceHandleUncaughtError( const WGPUErrorType ewgpuErrorType, const char * const pszMessage, void *const pUserdata )
    {
        logger::Info( "%s: type: %s, %s" ENDL, __FUNCTION__, WGPUErrorTypeToString( ewgpuErrorType ), pszMessage );
    }

    WGPUSwapChain CreateSwapChainForWindowDimensions( TitaniumPhysicalRenderingDevice *const pRendererDevice, TitaniumRendererState *const pRendererState, const ::util::maths::Vec2<u32> vWindowSize )
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
            /::util::data::StaticSpan<WGPUPresentMode, 4> sPresetModes;
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
        wgpuSwapchainFormat = wgpuSurfaceGetPreferredFormat( pRendererState->m_wgpuRenderSurface, pRendererDevice->m_wgpuAdapter );
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

    DepthTextureAndView CreateDepthTextureAndViewForWindowSize( TitaniumRendererState *const pRendererState, const ::util::maths::Vec2<u32> vWindowSize )
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
            .aspect = WGPUTextureAspect_All
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

    WGPUBindGroupLayout CreateBindGroupLayout( TitaniumRendererState *const pRendererState, ::util::data::Span<WGPUBindGroupLayoutEntry> swgpuBindGroupLayoutEntries )
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

        WGPUInstance wgpuInstance = pRendererDevice->m_wgpuInstance;
        WGPUAdapter wgpuAdapter = pRendererDevice->m_wgpuAdapter;

        const ::util::maths::Vec2<u32> vWindowSize = sys::sdl::GetWindowSizeVector( psdlWindow );
        WGPUSurface wgpuSurface = pRendererState->m_wgpuRenderSurface = sys::sdl::CreateWGPUSurfaceForWindow( psdlWindow, wgpuInstance );

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

        // Request queue from virtual device
        WGPUQueue wgpuQueue = pRendererState->m_wgpuQueue = wgpuDeviceGetQueue( wgpuVirtualDevice );
       
        // Request swapchain from device
        pRendererState->m_wgpuSwapChain = CreateSwapChainForWindowDimensions( pRendererDevice, pRendererState, vWindowSize );

        // TODO: temp, this should probably be provided by caller, rather than defined in renderer
        // create uniform bindgroup layout, this defines the way uniforms are laid out in the render pipeline
        WGPUBindGroupLayout wgpuBindGroupLayout = CreateBindGroupLayout( pRendererState, ::util::data::StaticSpan<WGPUBindGroupLayoutEntry, 1> {
            {
                .binding = 0,
                .visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Vertex,

                .buffer {
                    .type = WGPUBufferBindingType_Uniform,
                    .minBindingSize = sizeof( UShaderGlobals )
                }
            }
        }.ToConstSpan() );

        // create global uniform buffer, and it's bindgroup 
        // TODO: could we macro this syntax maybe?
        pRendererState->m_globalUniformBuffer = [ wgpuVirtualDevice, wgpuQueue, wgpuBindGroupLayout, vWindowSize ](){            
            WGPUBufferDescriptor wgpuUniformBufferDescriptor {
                .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
                .size = sizeof( UShaderGlobals )  
            };

            WGPUBuffer wgpuUniformBuffer = wgpuDeviceCreateBuffer( wgpuVirtualDevice, &wgpuUniformBufferDescriptor );   

            // write defaults
            UShaderGlobals uDefaultShaderGlobals { .m_vWindowSize = vWindowSize };
            wgpuQueueWriteBuffer( wgpuQueue, wgpuUniformBuffer, 0, &uDefaultShaderGlobals, sizeof( uDefaultShaderGlobals ) ); 

            WGPUBindGroupEntry wgpuBinding {
                .binding = 0,
                .buffer = wgpuUniformBuffer,
                .offset = 0,
                .size = sizeof( UShaderGlobals )
            };

            WGPUBindGroupDescriptor wgpuBindGroupDescriptor {
                .layout = wgpuBindGroupLayout,
                .entryCount = 1, // keep same as wgpuBindGroupLayoutDescriptor.entryCount!
                .entries = &wgpuBinding
            };

            WGPUBindGroup r_wgpuBindGroup = wgpuDeviceCreateBindGroup( wgpuVirtualDevice, &wgpuBindGroupDescriptor );

            return BufferAndBindgroup { .m_wgpuBindGroup = r_wgpuBindGroup, .m_wgpuBuffer = wgpuUniformBuffer };
        }();

        WGPUBindGroupLayout wgpuStandardObjectUniformBindGroupLayout = pRendererState->m_wgpuStandardObjectUniformBindGroupLayout = CreateBindGroupLayout( pRendererState, ::util::data::StaticSpan<WGPUBindGroupLayoutEntry, 1> {
            {
                .binding = 0,
                .visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Vertex,

                .buffer {
                    .type = WGPUBufferBindingType_Uniform,
                    .minBindingSize = sizeof( UShaderStandardVars )
                }
            }
        }.ToConstSpan() );

        // create error scope for pipeline creation so we can react to it
        wgpuDevicePushErrorScope( wgpuVirtualDevice, WGPUErrorFilter_Validation );
        {
            // Create render pipeline for device
            pRendererState->m_wgpuRenderPipeline = [ wgpuVirtualDevice, wgpuSwapchainFormat, wgpuBindGroupLayout, wgpuStandardObjectUniformBindGroupLayout ]()
            {
                // Create shader module
                WGPUShaderModule wgpuShaderModule = [ wgpuVirtualDevice ]()
                {
                    // TODO: need projection matrices in uniforms
                    WGPUShaderModuleWGSLDescriptor wgpuShaderCodeDescriptor {
                        .chain { .sType = WGPUSType_ShaderModuleWGSLDescriptor },
                        .code = R"(
                                    const PI = 3.14159265359;

                                    struct UShaderGlobals
                                    {
                                        mat4fCameraTransform : mat4x4<f32>,
                                        flTime : f32,
                                        vWindowSize : vec2<u32>
                                    };

                                    struct UShaderStandardVars
                                    {
                                        mat4fBaseTransform : mat4x4<f32>
                                    };

                                    @group( 0 ) @binding( 0 ) var<uniform> u_globals : UShaderGlobals;
                                    @group( 1 ) @binding( 0 ) var<uniform> u_standardInput : UShaderStandardVars;

                                    struct R_VertexOutput
                                    {
                                        @builtin( position ) position : vec4<f32>,
                                        @location( 0 ) colour : vec3<f32>
                                    };

                                    @vertex fn vs_main( @location( 0 ) vertexPosition : vec3<f32> ) -> R_VertexOutput
                                    {
                                        let flViewAngle = 3.0 * PI / 4.0; // three 8th of turn (1 turn = 2 pi)
                                        let flViewAngleC = cos( flViewAngle );
                                        let flViewAngleS = sin( flViewAngle );

                                        let vFocalPoint = vec3<f32>( 0.0, 0.0, 200.0 );

                                        let MViewTransform =
                                        // focal point
                                        transpose( mat4x4<f32> (
                                            1.0, 0.0, 0.0, vFocalPoint.x,
                                            0.0, 1.0, 0.0, vFocalPoint.y,
                                            0.0, 0.0, 1.0, vFocalPoint.z,
                                            0.0, 0.0, 0.0, 1.0
                                        ) ) *

                                        // rotate the viewpoint in the YZ plane
                                        transpose( mat4x4<f32> (
                                            1.0, 0.0,           0.0,            0.0,
                                            0.0, flViewAngleC,  flViewAngleS,   0.0,
                                            0.0, -flViewAngleS, flViewAngleC,   0.0,
                                            0.0, 0.0,           0.0,            1.0
                                        ) );

                                        let flAspectRatio = f32( u_globals.vWindowSize.x ) / f32( u_globals.vWindowSize.y );
                                        let flFocalLength = 2.0;
                                        let flNearDist = 0.01;
                                        let flFarDist = 1000.0;
                                        let flDivides = 1.0 / ( flFarDist - flNearDist );
                                        let MProjectFocal = transpose( mat4x4<f32>(
                                            flFocalLength, 0.0,                           0.0,                   0.0,
                                            0.0,           flFocalLength * flAspectRatio, 0.0,                   0.0,
                                            0.0,           0.0,                           flFarDist * flDivides, -flFarDist * flNearDist * flDivides,
                                            0.0,           0.0,                           1.0,                   0.0
                                        ) );

                                        var r_out : R_VertexOutput;
                                        r_out.position = MProjectFocal * MViewTransform * u_standardInput.mat4fBaseTransform * vec4<f32>( vertexPosition, 1.0 );
                                        r_out.colour = vec3<f32>( 1.0, 1.0, 1.0 ); // r_out.position.xyz * cos( u_globals.flTime );

                                        return r_out;
                                    }

                                    @fragment fn fs_main( in : R_VertexOutput ) -> @location( 0 ) vec4<f32> 
                                    {
                                        return vec4<f32>( in.position.xyz, 1.0 ); // vec4<f32>( pow( in.colour + ( sin( u_globals.flTime ) * 0.4 ), vec3<f32>( 2.2 ) ), 1.0 );
                                    }
                                )"
                    };

                    WGPUShaderModuleDescriptor wgpuShaderDescriptor {
                        .nextInChain = &wgpuShaderCodeDescriptor.chain
                    };

                    return wgpuDeviceCreateShaderModule( wgpuVirtualDevice, &wgpuShaderDescriptor );
                }();

                ::util::data::StaticSpan<WGPUBindGroupLayout, 2> swgpuBindGroupLayouts {
                    wgpuBindGroupLayout,
                    wgpuStandardObjectUniformBindGroupLayout
                };

                // create pipeline layout
                WGPUPipelineLayoutDescriptor wgpuPipelineLayoutDescriptor {
                    .bindGroupLayoutCount = static_cast<u32>( swgpuBindGroupLayouts.Elements() ),
                    .bindGroupLayouts = swgpuBindGroupLayouts.m_tData
                };

                WGPUPipelineLayout wgpuPipelineLayout =  wgpuDeviceCreatePipelineLayout( wgpuVirtualDevice, &wgpuPipelineLayoutDescriptor );

                ::util::data::StaticSpan<WGPUVertexAttribute, 1> sVertexAttributes {
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
        } 

        bool bPipelineCompilationFailed = false;
        wgpuDevicePopErrorScope( wgpuVirtualDevice, []( const WGPUErrorType ewgpuErrorType, const char * const pszMessage, void *const bPipelineCompilationFailed ){ 
            logger::Info( "Pipeline creation failed with message: %s" ENDL, pszMessage );
            *static_cast<bool *>( bPipelineCompilationFailed ) = true;
        }, &bPipelineCompilationFailed );

        pRendererState->m_depthTextureAndView = CreateDepthTextureAndViewForWindowSize( pRendererState, vWindowSize );

        // imgui init
        // TODO: what's the significance of having multiple frames in flight??? is this important??? idk doing 1
        ImGui_ImplWGPU_Init( wgpuVirtualDevice, 1, wgpuSwapchainFormat, WGPUTextureFormat_Depth24Plus );

        logger::Info( "wgpu renderer initialised successfully!" ENDL );
    }

    void preframe::ResolutionChanged( TitaniumPhysicalRenderingDevice *const pRendererDevice, TitaniumRendererState *const pRendererState, const ::util::maths::Vec2<u32> vWindowSize )
    {
        // swapchains rely on the window's resolution, so need to be recreated on window resize
        wgpuSwapChainRelease( pRendererState->m_wgpuSwapChain ); // destroy old swapchain
        pRendererState->m_wgpuSwapChain = CreateSwapChainForWindowDimensions( pRendererDevice, pRendererState, vWindowSize );

        FreeDepthTextureAndView( &pRendererState->m_depthTextureAndView );
        pRendererState->m_depthTextureAndView = CreateDepthTextureAndViewForWindowSize( pRendererState, vWindowSize );

        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, pRendererState->m_globalUniformBuffer.m_wgpuBuffer, offsetof( UShaderGlobals, m_vWindowSize ), &vWindowSize, sizeof( ::util::maths::Vec2<u32> ) );
    }

    void preframe::ImGUI( TitaniumRendererState *const pRendererState )
    {
        ImGui_ImplWGPU_NewFrame();
    }

    GPUModelHandle UploadModel( TitaniumRendererState *const pRendererState, const ::util::data::Span<float> sflVertices, const ::util::data::Span<int> snIndexes )
    {
        const size_t nVertexBufSize = sflVertices.m_nElements * sizeof( float );
        const size_t nIndexBufSize = snIndexes.m_nElements * sizeof( int );
            
        WGPUBufferDescriptor wgpuVertexBufferDescriptor {
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .size = nVertexBufSize,
        };

        WGPUBuffer wgpuVertexBuffer = wgpuDeviceCreateBuffer( pRendererState->m_wgpuVirtualDevice, &wgpuVertexBufferDescriptor );
        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, wgpuVertexBuffer, 0, sflVertices.m_pData, nVertexBufSize );

        WGPUBufferDescriptor wgpuIndexBufferDescriptor {
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
            .size = nIndexBufSize 
        };

        WGPUBuffer wgpuIndexBuffer = wgpuDeviceCreateBuffer( pRendererState->m_wgpuVirtualDevice, &wgpuIndexBufferDescriptor );
        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, wgpuIndexBuffer, 0, snIndexes.m_pData, nIndexBufSize );

        return { 
            wgpuVertexBuffer, nVertexBufSize,
            wgpuIndexBuffer, nIndexBufSize, static_cast<int>( snIndexes.m_nElements )
        };
    }

    void FreeGPUModel( GPUModelHandle gpuModel )
    {
        wgpuBufferDestroy( gpuModel.m_wgpuVertexBuffer );
        wgpuBufferRelease( gpuModel.m_wgpuVertexBuffer );

        wgpuBufferDestroy( gpuModel.m_wgpuIndexBuffer );
        wgpuBufferRelease( gpuModel.m_wgpuIndexBuffer );
    }

    void CreateRenderableObjectBuffers( TitaniumRendererState *const pRendererState, RenderableObject *const pRenderableObject )
    {
        // TODO: this method of creating buffers kind of sucks, would be nice if there was a way to just map these to c structs at runtime
        WGPUBufferDescriptor wgpuStandardUniformBufferDescriptor {
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
            .size = sizeof( UShaderStandardVars )
        };

        WGPUBuffer wgpuUniformBuffer = wgpuDeviceCreateBuffer( pRendererState->m_wgpuVirtualDevice, &wgpuStandardUniformBufferDescriptor );

        WGPUBindGroupEntry wgpuBinding {
            .binding = 0,
            .buffer = wgpuUniformBuffer,
            .offset = 0,
            .size = sizeof( UShaderStandardVars )
        };

        WGPUBindGroupDescriptor wgpuBindGroupDescriptor {
            .layout = pRendererState->m_wgpuStandardObjectUniformBindGroupLayout,
            .entryCount = 1,
            .entries = &wgpuBinding
        };

        WGPUBindGroup r_wgpuBindGroup = wgpuDeviceCreateBindGroup( pRendererState->m_wgpuVirtualDevice, &wgpuBindGroupDescriptor );

        pRenderableObject->m_standardUniforms = { .m_wgpuBindGroup = r_wgpuBindGroup, .m_wgpuBuffer = wgpuUniformBuffer };
    }

    void FreeRenderableObjectBuffers( RenderableObject *const pRenderableObject )
    {
        wgpuBindGroupRelease( pRenderableObject->m_standardUniforms.m_wgpuBindGroup );

        wgpuBufferDestroy( pRenderableObject->m_standardUniforms.m_wgpuBuffer );
        wgpuBufferRelease( pRenderableObject->m_standardUniforms.m_wgpuBuffer );
    }

    void Frame( TitaniumRendererState *const pRendererState, const ::util::data::Span<RenderableObject> sRenderableObjects )
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
        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, pRendererState->m_globalUniformBuffer.m_wgpuBuffer, offsetof( UShaderGlobals, m_flTime ), &flCurrentTime, sizeof( f32 ) );

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
            wgpuRenderPassEncoderSetPipeline( wgpuRenderPass, pRendererState->m_wgpuRenderPipeline );
            wgpuRenderPassEncoderSetBindGroup( wgpuRenderPass, 0, pRendererState->m_globalUniformBuffer.m_wgpuBindGroup, 0, nullptr );

            for ( int i = 0; i < sRenderableObjects.m_nElements; i++ )
            {
                const RenderableObject *const pRenderableObject = &sRenderableObjects.m_pData[ i ];


                // TODO: should check if we need to write this (dirty bit)
                glm::mat4x4 mat4fTransform = glm::eulerAngleXYZ( pRenderableObject->m_vRotation.x, pRenderableObject->m_vRotation.y, pRenderableObject->m_vRotation.z );
                glm::translate( mat4fTransform, glm::vec3( pRenderableObject->m_vPosition.x, pRenderableObject->m_vPosition.y, pRenderableObject->m_vPosition.z  ) );

                wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, sRenderableObjects.m_pData[ i ].m_standardUniforms.m_wgpuBuffer, offsetof( UShaderStandardVars, m_mat4fBaseTransform ), &mat4fTransform, sizeof( mat4fTransform ) );

                wgpuRenderPassEncoderSetBindGroup( wgpuRenderPass, 1, sRenderableObjects.m_pData[ i ].m_standardUniforms.m_wgpuBindGroup, 0, nullptr );

                // TODO: make uint16 again? from uint32
                wgpuRenderPassEncoderSetVertexBuffer( wgpuRenderPass, 0, sRenderableObjects.m_pData[ i ].m_gpuModel.m_wgpuVertexBuffer, 0, sRenderableObjects.m_pData[ i ].m_gpuModel.m_nVertexBufferSize );
                wgpuRenderPassEncoderSetIndexBuffer( wgpuRenderPass, sRenderableObjects.m_pData[ i ].m_gpuModel.m_wgpuIndexBuffer, WGPUIndexFormat_Uint32, 0, sRenderableObjects.m_pData[ i ].m_gpuModel.m_nIndexBufferSize );
                wgpuRenderPassEncoderDrawIndexed( wgpuRenderPass, sRenderableObjects.m_pData[ i ].m_gpuModel.m_nIndexBufferCount, 1, 0, 0, 0 );
            }
    
            // imgui
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
