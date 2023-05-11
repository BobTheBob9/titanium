#include "vk_utils.hpp"

#include <vulkan/vk_enum_string_helper.h> 

#include "titanium/logger/logger.hpp"

namespace renderer::vulkan
{
    void vkAssertResult( const VkResult eResult, const char *const pszFuncName )
    {
        if ( eResult != VkResult::VK_SUCCESS ) [[ unlikely ]]
        {
            // TODO: fail
            logger::Info( "Vulkan call \"%s\" failed with return code %s" ENDL, pszFuncName, string_VkResult( eResult ) );
        }
    }

    void vkAssertOther( const bool bResult )
    {
        
    }
};
