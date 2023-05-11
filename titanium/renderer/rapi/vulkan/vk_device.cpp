#include "vk_device.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "titanium/logger/logger.hpp"
#include "titanium/memory/mem_core.hpp"
#include "titanium/util/data/vector.hpp"
#include "titanium/util/numerics.hpp"
#include "titanium/util/data/span.hpp"

#include "titanium/renderer/rapi/vulkan/vk_utils.hpp"

namespace renderer::vulkan
{
    utils::data::Span< R_vkRenderDevice_s > QueryRenderDevices( const VkInstance vkInstance )
    {
        utils::data::Span< VkPhysicalDevice > svkDevices;
        VK_CALL( vkEnumeratePhysicalDevices( vkInstance, &svkDevices.m_nElements, nullptr ) );
        vkAssertOther( svkDevices.m_nElements ); // ensure we have any render devices at all
        svkDevices.m_pData = memory::alloc_nT<VkPhysicalDevice>( svkDevices.m_nElements );
        VK_CALL( vkEnumeratePhysicalDevices( vkInstance, &svkDevices.m_nElements, svkDevices.m_pData ) );

        utils::data::Span< R_vkRenderDevice_s > r_svkRenderDevices( svkDevices.m_nElements, memory::alloc_nT< R_vkRenderDevice_s >( svkDevices.m_nElements ) );
        for ( int i = 0; i < r_svkRenderDevices.m_nElements; i++ )
        {
            VkPhysicalDeviceProperties vkDeviceProperties;
            vkGetPhysicalDeviceProperties( svkDevices.m_pData[ i ], &vkDeviceProperties );

            r_svkRenderDevices.m_pData[ i ] = { 
                .vkPhysicalDevice = svkDevices.m_pData[ i ], 
                .vkDeviceProperties = vkDeviceProperties 
            };
        }

        return r_svkRenderDevices;
    }


    R_vkRenderDevice_s PickRenderDevice( const utils::data::Span< R_vkRenderDevice_s > svkRenderDevices )
    {
        vkAssertOther( svkRenderDevices.m_nElements ); // ensure we have any render devices at all

        for ( int i = 0; i < svkRenderDevices.m_nElements; i++ )
        {
            // TODO: we should have somewhat more complex logic for picking this, maybe we could allow users to pick them on renderer reinitialisation?
            // maybe we should have some logic for prioritising different deviceTypes also ( maybe discrete => integrated => etc )
            if ( svkRenderDevices.m_pData[ i ].vkDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
                return svkRenderDevices.m_pData[ i ];
        }

        // TODO: warn here
        // no discrete gpu found, use the default
        return svkRenderDevices.m_pData[ 0 ];
    }

    void LogRenderDeviceInformation( const R_vkRenderDevice_s vkRenderDevice )
    {
        logger::Info( "%s: Render Device %s is selected" ENDL, __PRETTY_FUNCTION__, vkRenderDevice.vkDeviceProperties.deviceName );

        // log information on the selected render device
        // TODO: no logger :c
    }
};
