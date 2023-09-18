#include "renderer.hpp"
#include "renderer_uniforms.hpp"
#include "util/data/staticspan.hpp"
#include "util/maths.hpp"

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace renderer
{
    glm::mat4x4 GLM_YawPitchRoll_ZUpFromDegrees( util::maths::Vec3<f32> vfObjectRotation )
    {
        return glm::yawPitchRoll( glm::radians( vfObjectRotation.z ), glm::radians( -vfObjectRotation.y ), glm::radians( -vfObjectRotation.x ) );
    }

    void RenderView_Create( TitaniumRendererState *const pRendererState, RenderView *const pRenderView )
    {
        // make buffer
        WGPUBufferDescriptor wgpuUniformBufferDescriptor {
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
            .size = sizeof( UShaderView )
        };
        WGPUBuffer wgpuUniformBuffer = wgpuDeviceCreateBuffer( pRendererState->m_wgpuVirtualDevice, &wgpuUniformBufferDescriptor );

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

        pRenderView->m_viewUniforms = { .m_wgpuBindGroup = r_wgpuBindGroup, .m_wgpuBuffer = wgpuUniformBuffer };
        RenderView_WriteToUniformBuffer( pRendererState, pRenderView );
    }

	void RenderView_Free( RenderView *const pRenderView )
    {
       wgpuBufferDestroy( pRenderView->m_viewUniforms.m_wgpuBuffer );
       wgpuBufferRelease( pRenderView->m_viewUniforms.m_wgpuBuffer );
       wgpuBindGroupRelease( pRenderView->m_viewUniforms.m_wgpuBindGroup );
    }

    /*
     *  Write all modified variables to the renderview's gpu uniform buffer
     */
    void RenderView_WriteToUniformBuffer( TitaniumRendererState *const pRendererState, RenderView *const pRenderView )
    {
        glm::mat4x4 mat4fTransform = GLM_YawPitchRoll_ZUpFromDegrees( pRenderView->m_vCameraRotation );
        mat4fTransform = glm::translate( mat4fTransform, glm::vec3( pRenderView->m_vCameraPosition.x, pRenderView->m_vCameraPosition.y, pRenderView->m_vCameraPosition.z ) );

        f32 flAspectRatio = f32( pRenderView->m_vRenderResolution.x ) / f32( pRenderView->m_vRenderResolution.y );
        f32 flNearDist = 0.01;
        f32 flFarDist = 10000.0;
        glm::mat4x4 mat4fProjectFocal = glm::perspective( 1.5f, flAspectRatio, flNearDist, flFarDist );

        const glm::mat4x4 mat4fCamera = mat4fProjectFocal * mat4fTransform;
        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, pRenderView->m_viewUniforms.m_wgpuBuffer, offsetof( UShaderView, m_mat4fCameraTransform ), &mat4fCamera, sizeof( mat4fCamera ) );
        pRenderView->m_bGPUDirty = false;
    }

    void RenderObject_Create( TitaniumRendererState *const pRendererState, RenderObject *const pRenderObject )
    {
        // make buffer
        // TODO: this method of creating buffers kind of sucks, would be nice if there was a way to just map these to c structs at runtime
        WGPUBufferDescriptor wgpuStandardUniformBufferDescriptor {
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
            .size = sizeof( UShaderObjectInstance )
        };
        WGPUBuffer wgpuUniformBuffer = wgpuDeviceCreateBuffer( pRendererState->m_wgpuVirtualDevice, &wgpuStandardUniformBufferDescriptor );

        // make binding to buffer
        util::data::StaticSpan<WGPUBindGroupEntry, 3> wgpuObjectBindings {
            {
                .binding = 0,
                .buffer = wgpuUniformBuffer,
                .size = sizeof( UShaderObjectInstance )
            },
            {
                .binding = 1,
                .textureView = pRenderObject->m_gpuTexture.m_wgpuTextureView
            },
            {
                .binding = 2,
                .sampler = pRendererState->m_wgpuTextureSampler
            }
        };

        WGPUBindGroupDescriptor wgpuBindGroupDescriptor {
            .layout = pRendererState->m_wgpuUniformBindGroupLayout_UShaderObjectInstance,
            .entryCount = static_cast<u32>( wgpuObjectBindings.Elements() ),
            .entries = wgpuObjectBindings.m_tData
        };
        WGPUBindGroup r_wgpuBindGroup = wgpuDeviceCreateBindGroup( pRendererState->m_wgpuVirtualDevice, &wgpuBindGroupDescriptor );

        pRenderObject->m_objectUniforms = { .m_wgpuBindGroup = r_wgpuBindGroup, .m_wgpuBuffer = wgpuUniformBuffer };
        RenderObject_WriteToUniformBuffer( pRendererState, pRenderObject );
    }

    void RenderObject_Free( RenderObject *const pRenderObject )
    {
        wgpuBufferDestroy( pRenderObject->m_objectUniforms.m_wgpuBuffer );
        wgpuBufferRelease( pRenderObject->m_objectUniforms.m_wgpuBuffer );
        wgpuBindGroupRelease( pRenderObject->m_objectUniforms.m_wgpuBindGroup );
    }

    void RenderObject_WriteToUniformBuffer( TitaniumRendererState *const pRendererState, RenderObject *const pRenderObject )
    {
        glm::mat4x4 mat4fTransform = GLM_YawPitchRoll_ZUpFromDegrees( pRenderObject->m_vRotation );
        mat4fTransform = glm::translate( mat4fTransform, glm::vec3( pRenderObject->m_vPosition.x, pRenderObject->m_vPosition.y, pRenderObject->m_vPosition.z  ) );
        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, pRenderObject->m_objectUniforms.m_wgpuBuffer, offsetof( UShaderObjectInstance, m_mat4fBaseTransform ),  &mat4fTransform, sizeof( mat4fTransform ) );

        pRenderObject->m_bGPUDirty = false; // gpu has up-to-date state for the object
    }
}
