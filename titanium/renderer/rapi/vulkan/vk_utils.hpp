#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace renderer::vulkan
{
    void vkAssertResult( const VkResult vkeResult, const char *const pszFuncName );
    void vkAssertOther( const bool bResult );
    void vkAssertOther32( const VkBool32 vkbResult );

    void vkAssertResultWarn( const VkResult vkeResult );
    void vkAssertOtherWarn( const bool bResult );
    void vkAssertOther32Warn( const VkBool32 vkbResult );
};

#define VK_CALL( func ) vkAssertResult( func, #func )
