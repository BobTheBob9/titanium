#include "vk_main.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <GLFW/glfw3.h> 

#include "titanium/logger/logger.hpp"
#include "titanium/memory/mem_core.hpp"
#include "titanium/util/assert.hpp"
#include "titanium/util/numerics.hpp"
#include "titanium/util/data/span.hpp"
#include "titanium/util/data/vector.hpp"

#include "titanium/renderer/rapi/vulkan/vk_utils.hpp"
#include "titanium/renderer/rapi/vulkan/vk_device.hpp"

namespace renderer::vulkan
{
    VkBool32 vkDebugMessageCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
    {
        // TODO: impl
        return VK_TRUE;
    }

    VkInstance CreateInstance()
    {
        utils::data::Vector< const char * > vpszEnabledInstanceLayers;
        {
            // query all available instance layers, to ensure we can use all the ones we need
            utils::data::Span< VkLayerProperties > svkAvailableLayerProperties;
            VK_CALL( vkEnumerateInstanceLayerProperties( &svkAvailableLayerProperties.m_nElements, nullptr ) );
            svkAvailableLayerProperties.m_pData = memory::alloc_nT< VkLayerProperties >( svkAvailableLayerProperties.m_nElements );
            VK_CALL( vkEnumerateInstanceLayerProperties( &svkAvailableLayerProperties.m_nElements, svkAvailableLayerProperties.m_pData ) );

            // TODO: validation layer
            #if VK_DEBUG

            #endif

            // ensure we can use all the layers we need
            for ( int i = 0; i < vpszEnabledInstanceLayers.Length(); i++ )
            {
                // TODO: validate necessary layers
                //vkAssertOther( svkAvailableLayerProperties.Contains( vpszEnabledInstanceLayers[ i ] ) );
            }
        }

        utils::data::Vector< const char * > vpszEnabledExtensions;
        {
            // add all extensions we need
            vpszEnabledExtensions.AppendWithAlloc( VK_KHR_SURFACE_EXTENSION_NAME );
            vpszEnabledExtensions.AppendWithAlloc( VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME );

            utils::data::Span< const char* > sglfwRequiredVulkanExtensions;
            sglfwRequiredVulkanExtensions.m_pData = glfwGetRequiredInstanceExtensions( &sglfwRequiredVulkanExtensions.m_nElements );
            vpszEnabledExtensions.AppendMultipleWithAlloc( sglfwRequiredVulkanExtensions );

            // query all usable instance extensions, to ensure we can use all the ones we need
            utils::data::Span< VkExtensionProperties > svkAvailableExtensionProperties;
            VK_CALL( vkEnumerateInstanceExtensionProperties( nullptr, &svkAvailableExtensionProperties.m_nElements, nullptr ) );
            svkAvailableExtensionProperties.m_pData = memory::alloc_nT< VkExtensionProperties >( svkAvailableExtensionProperties.m_nElements );
            VK_CALL( vkEnumerateInstanceExtensionProperties( nullptr, &svkAvailableExtensionProperties.m_nElements, svkAvailableExtensionProperties.m_pData ) );
    
            // ensure we can use all the extensions we need
            for ( int i = 0; i < vpszEnabledExtensions.Length(); i++ )
            {
                // TODO: validate necessary extensions
                //vkAssertOther( svkAvailableExtensionProperties.Contains( vpszEnabledExtensions.GetAt( i ) ) );
            }
        }

        // vulkan application info
        VkApplicationInfo vkAppInfo { 
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, 
            .pApplicationName = "Titanium - UNKNOWN GAME", // TODO: add actual application name
            .applicationVersion = 0,
            .pEngineName = "Titanium - Vulkan Renderer \"Rose\"",
            .engineVersion = 0,
            .apiVersion = VK_MAKE_VERSION( 1, 3, 0 )
        };

        // instance info
        VkInstanceCreateInfo vkInstanceInfo = { 
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &vkAppInfo,

            // TODO
            .enabledLayerCount = vpszEnabledInstanceLayers.Length(),
            .ppEnabledLayerNames = vpszEnabledInstanceLayers.DataForReadOnly(),
            .enabledExtensionCount = vpszEnabledExtensions.Length(),
            .ppEnabledExtensionNames = vpszEnabledExtensions.DataForReadOnly(),
        };

        #if VK_DEBUG
            VkDebugUtilsMessengerCreateInfoEXT vkDebugMessageCreateInfo { 
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
                .pfnUserCallback = vkDebugMessageCallback
            };

            vkInstanceInfo.pNext = &vkDebugMessageCreateInfo;
        #endif // #if VK_DEBUG

        // actually construct the vkInstance
        VkInstance r_vkInstance;
        VK_CALL( vkCreateInstance( &vkInstanceInfo, nullptr, &r_vkInstance ) );

        return r_vkInstance;
    }
    
    void Initialise()
    {
        glfwInit();
        LOG_CALL( VkInstance vkInstance = CreateInstance() );

        // Pick render device
        utils::data::Span< R_vkRenderDevice_s > svkRenderDevices = QueryRenderDevices( vkInstance );
        R_vkRenderDevice_s vkRenderDevice = PickRenderDevice( svkRenderDevices );
        LogRenderDeviceInformation( vkRenderDevice );

        GLFWwindow* pglfwWindow;
        VkSurfaceKHR vkSurface;
        assert::Release( glfwVulkanSupported() == GLFW_TRUE );
        VK_CALL(glfwCreateWindowSurface( vkInstance, pglfwWindow, nullptr, &vkSurface ) );

    }
};
