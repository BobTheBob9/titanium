#include "renderer.hpp"
#include "renderer_uniforms.hpp"

namespace renderer
{
    void CreateRenderView( TitaniumRendererState *const pRendererState, RenderView *const pRenderView, const util::maths::Vec2<u32> vWindowSize )
    {
        // make buffer
        WGPUBufferDescriptor wgpuUniformBufferDescriptor {
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
            .size = sizeof( UShaderView )
        };
        WGPUBuffer wgpuUniformBuffer = wgpuDeviceCreateBuffer( pRendererState->m_wgpuVirtualDevice, &wgpuUniformBufferDescriptor );

        // write defaults
        // TODO: temp, should just do this on m_bGPUDirty
        // or not? should probably reevaluate how we initialise gpu objects, but regardless it should be the same across all types of object
        pRenderView->m_vRenderResolution = vWindowSize;
        UShaderView uDefaultShaderGlobals { .m_vWindowSize = vWindowSize };
        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, wgpuUniformBuffer, 0, &uDefaultShaderGlobals, sizeof( uDefaultShaderGlobals ) );

        // make binding to buffer
        WGPUBindGroupEntry wgpuBinding {
            .binding = 0,
            .buffer = wgpuUniformBuffer,
            .offset = 0,
            .size = sizeof( UShaderView )
        };
        WGPUBindGroupDescriptor wgpuBindGroupDescriptor {
            .layout = pRendererState->m_wgpuUniformBindGroupLayout_UShaderView,
            .entryCount = 1, // keep same as wgpuBindGroupLayoutDescriptor.entryCount!
            .entries = &wgpuBinding
        };
        WGPUBindGroup r_wgpuBindGroup = wgpuDeviceCreateBindGroup( pRendererState->m_wgpuVirtualDevice, &wgpuBindGroupDescriptor );

        pRenderView->m_bGPUDirty = true;
        pRenderView->m_viewUniforms = { .m_wgpuBindGroup = r_wgpuBindGroup, .m_wgpuBuffer = wgpuUniformBuffer };
    }

	void FreeRenderView( RenderView *const pRenderView )
    {
       wgpuBufferDestroy( pRenderView->m_viewUniforms.m_wgpuBuffer );
       wgpuBufferRelease( pRenderView->m_viewUniforms.m_wgpuBuffer );
       wgpuBindGroupRelease( pRenderView->m_viewUniforms.m_wgpuBindGroup );
    }

    void CreateRenderableObjectBuffers( TitaniumRendererState *const pRendererState, RenderableObject *const pRenderableObject )
    {
        // make buffer
        // TODO: this method of creating buffers kind of sucks, would be nice if there was a way to just map these to c structs at runtime
        WGPUBufferDescriptor wgpuStandardUniformBufferDescriptor {
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
            .size = sizeof( UShaderObjectInstance )
        };
        WGPUBuffer wgpuUniformBuffer = wgpuDeviceCreateBuffer( pRendererState->m_wgpuVirtualDevice, &wgpuStandardUniformBufferDescriptor );

        // make binding to buffer
        WGPUBindGroupEntry wgpuBinding {
            .binding = 0,
            .buffer = wgpuUniformBuffer,
            .offset = 0,
            .size = sizeof( UShaderObjectInstance )
        };
        WGPUBindGroupDescriptor wgpuBindGroupDescriptor {
            .layout = pRendererState->m_wgpuUniformBindGroupLayout_UShaderObjectInstance,
            .entryCount = 1,
            .entries = &wgpuBinding
        };
        WGPUBindGroup r_wgpuBindGroup = wgpuDeviceCreateBindGroup( pRendererState->m_wgpuVirtualDevice, &wgpuBindGroupDescriptor );

        pRenderableObject->m_bGPUDirty = true;
        pRenderableObject->m_objectUniforms = { .m_wgpuBindGroup = r_wgpuBindGroup, .m_wgpuBuffer = wgpuUniformBuffer };
    }

    void FreeRenderableObjectBuffers( RenderableObject *const pRenderableObject )
    {
        wgpuBufferDestroy( pRenderableObject->m_objectUniforms.m_wgpuBuffer );
        wgpuBufferRelease( pRenderableObject->m_objectUniforms.m_wgpuBuffer );
        wgpuBindGroupRelease( pRenderableObject->m_objectUniforms.m_wgpuBindGroup );
    }
}
