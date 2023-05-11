# renderer contains the renderer
# at the moment, this is only our vulkan renderer "Rose", based on the awesome flower renderer (https://github.com/qiutang98/flower)

add_subdirectory( rapi/vulkan )

include_directories(*)
add_sources(
    renderer.cpp
)