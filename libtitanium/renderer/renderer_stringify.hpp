#pragma once

#include <webgpu/webgpu.h>

#include <libtitanium/util/data/stringbuf.hpp>

namespace renderer
{
    const char *const WGPURequestAdapterStatusToString( const WGPURequestAdapterStatus ewgpuRequestAdapterStatus );
    const char *const WGPURequestDeviceStatusToString( const WGPURequestDeviceStatus ewgpuRequestDeviceStatus );
    const char *const WGPUAdapterTypeToString( const WGPUAdapterType ewgpuAdapterType );
    const char *const WGPUBackendTypeToString( const WGPUBackendType ewgpuBackendType );
    const char *const WGPUFeatureNameToString( const WGPUFeatureName ewgpuFeatureName );
    const char *const WGPUErrorTypeToString( const WGPUErrorType ewgpuErrorType );
    const char *const WGPUPresentModeToString( const WGPUPresentMode ewgpuPresentMode );
}
