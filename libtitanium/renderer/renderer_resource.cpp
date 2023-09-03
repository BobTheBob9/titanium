#include "renderer.hpp"

namespace renderer
{
    GPUModelHandle UploadModel( TitaniumRendererState *const pRendererState, const ::util::data::Span<float> sflVertices, const ::util::data::Span<u16> snIndexes )
    {
        const size_t nVertexBufSize = sflVertices.m_nElements * sizeof( float );
        WGPUBufferDescriptor wgpuVertexBufferDescriptor {
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .size = nVertexBufSize,
        };
        WGPUBuffer wgpuVertexBuffer = wgpuDeviceCreateBuffer( pRendererState->m_wgpuVirtualDevice, &wgpuVertexBufferDescriptor );
        wgpuQueueWriteBuffer( pRendererState->m_wgpuQueue, wgpuVertexBuffer, 0, sflVertices.m_pData, nVertexBufSize );

        const size_t nIndexBufSize = snIndexes.m_nElements * sizeof( u16 );
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
}
