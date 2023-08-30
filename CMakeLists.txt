cmake_minimum_required(VERSION 3.5)

project(titanium LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package( SDL2 REQUIRED ) 

set( WEBGPU_BACKEND WGPU )
#set( DAWN_USE_WAYLAND ON )
add_subdirectory( extern/WebGPU-distribution )
add_subdirectory( extern/imgui )
#add_subdirectory( extern/openal-soft )

option( BUILD_SHARED_LIBS OFF )
add_subdirectory( extern/assimp )

# lib stuff
# TODO: should be in it's own github project and/or file, probably
add_library( libtitanium_util INTERFACE )
target_include_directories( libtitanium_util INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libtitanium/util
    ${CMAKE_SOURCE_DIR}/libtitanium/util/data
)
target_sources( libtitanium_util INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/util/commandline.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/util/string.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/util/data/vector_tests.cpp
)
target_link_libraries( libtitanium_util INTERFACE libtitanium_memory )


add_library( libtitanium_memory INTERFACE )
target_include_directories( libtitanium_memory INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libtitanium/memory
)
target_sources( libtitanium_memory INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/memory/mem_core.cpp
)
target_link_libraries( libtitanium_memory INTERFACE libtitanium_util )


add_library( libtitanium_dev INTERFACE )
target_include_directories( libtitanium_dev INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libtitanium/dev
)
target_sources( libtitanium_dev INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/dev/tests.cpp
)
target_link_libraries( libtitanium_dev INTERFACE libtitanium_util libtitanium_logger )


add_library( libtitanium_config INTERFACE )
target_include_directories( libtitanium_config INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libtitanium/config
)
target_sources( libtitanium_config INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/config/config.cpp
)
target_link_libraries( libtitanium_config INTERFACE libtitanium_util )


add_library( libtitanium_logger INTERFACE )
target_include_directories( libtitanium_logger INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libtitanium/logger
)
target_sources( libtitanium_memory INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/logger/logger.cpp
)


add_library( libtitanium_sys INTERFACE )
target_include_directories( libtitanium_sys INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libtitanium/sys
)
target_sources( libtitanium_sys INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/sys/sys_sdl.cpp
)
target_link_libraries( libtitanium_sys INTERFACE libtitanium_util libtitanium_logger )


add_library( libtitanium_renderer INTERFACE )
target_include_directories( libtitanium_renderer INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libtitanium/renderer
)
target_sources( libtitanium_renderer INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/renderer/renderer.cpp
)
target_link_libraries( libtitanium_renderer INTERFACE libtitanium_util libtitanium_config libtitanium_logger imgui )


add_library( libtitanium_imguiwidgets INTERFACE )
target_include_directories( libtitanium_imguiwidgets INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libtitanium/imgui_widgets
)
target_sources( libtitanium_imguiwidgets INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/imgui_widgets/console.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/imgui_widgets/resourceprofiler.cpp
)
target_link_libraries( libtitanium_imguiwidgets INTERFACE imgui libtitanium_util libtitanium_config )


# TODO: VERY TEMP! this should be done in a fancy way of some kind
add_executable( titanium_bin game/game_main.cpp game/game_consolecommand.cpp )
target_include_directories( titanium_bin PUBLIC ${CMAKE_SOURCE_DIR}/libtitanium ${SDL2_INCLUDE_DIRS} )
target_link_libraries( titanium_bin PUBLIC webgpu assimp ${SDL2_LIBRARIES} imgui
    libtitanium_util
    libtitanium_memory
    libtitanium_dev
    libtitanium_config
    libtitanium_logger
    libtitanium_sys
    libtitanium_renderer
    libtitanium_imguiwidgets
)

install(TARGETS titanium_bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
