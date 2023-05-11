#include "renderer_api.hpp"

#include "titanium/logger/logger.hpp"

#include "titanium/renderer/rapi/vulkan/vk_main.hpp"

namespace renderer
{
    void Initialise()
    {
        LOG_CALL( vulkan::Initialise() );
    }
}
