# rapi/vulkan contains files for the implementation of our renderer using the vulkan rendering api

include_directories(*)
add_sources(
    vk_utils.cpp
    vk_main.cpp
    vk_device.cpp
)