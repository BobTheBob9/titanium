#pragma once

#include <vulkan/vulkan.h>

#include "titanium/util/data/span.hpp"

namespace renderer::vulkan
{
    struct R_vkRenderDevice_s
    {
        VkPhysicalDevice vkPhysicalDevice;
        VkPhysicalDeviceProperties vkDeviceProperties;
    };

    /*
    
    MEM: Return value must be freed by caller

    */
    utils::data::Span< R_vkRenderDevice_s > QueryRenderDevices( const VkInstance vkInstance );
    R_vkRenderDevice_s PickRenderDevice( const utils::data::Span< R_vkRenderDevice_s > svkRenderDevices );
    void LogRenderDeviceInformation( const R_vkRenderDevice_s vkRenderDevice );
};
