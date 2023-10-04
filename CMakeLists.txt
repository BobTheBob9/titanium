cmake_minimum_required( VERSION 3.5 )
project( titanium LANGUAGES CXX )

option( TITANIUM_WARNINGS_AS_ERRORS OFF )
if ( TITANIUM_WARNINGS_AS_ERRORS )
    set( TITANIUM_COMPILER_ARGS -Wall -Wextra -Wpedantic -Werror )
else()
    set( TITANIUM_COMPILER_ARGS -Wall -Wextra -Wpedantic )
endif()

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

set( CMAKE_CXX_STANDARD 20)
set( CMAKE_CXX_STANDARD_REQUIRED ON )

find_package( SDL2 REQUIRED )
find_package( OpenAL REQUIRED )
find_package( glm REQUIRED )

option( ASSIMP_BUILD_ZLIB ON )
option( BUILD_SHARED_LIBS OFF )
find_package( assimp REQUIRED )

set( WEBGPU_BACKEND WGPU )
#set( DAWN_USE_WAYLAND ON )
add_subdirectory( extern/WebGPU-distribution )
add_subdirectory( extern/imgui )


# lib stuff
# TODO: should be in it's own github project and/or file, probably
add_library( libtitanium_util INTERFACE )
target_include_directories( libtitanium_util INTERFACE
    ${CMAKE_SOURCE_DIR}
)
target_sources( libtitanium_util INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/util/commandline.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/util/string.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/util/data/vector_tests.cpp
)
target_link_libraries( libtitanium_util INTERFACE libtitanium_memory )
target_compile_options( libtitanium_util INTERFACE ${TITANIUM_COMPILER_ARGS} )


add_library( libtitanium_memory INTERFACE )
target_include_directories( libtitanium_memory INTERFACE
    ${CMAKE_SOURCE_DIR}
)
target_sources( libtitanium_memory INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/memory/mem_core.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/memory/mem_external.cpp
)
target_link_libraries( libtitanium_memory INTERFACE libtitanium_util )
target_compile_options( libtitanium_memory INTERFACE ${TITANIUM_COMPILER_ARGS} )


add_library( libtitanium_dev INTERFACE )
target_include_directories( libtitanium_dev INTERFACE
    ${CMAKE_SOURCE_DIR}
)
target_sources( libtitanium_dev INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/dev/tests.cpp
)
target_link_libraries( libtitanium_dev INTERFACE libtitanium_util libtitanium_logger )
target_compile_options( libtitanium_dev INTERFACE ${TITANIUM_COMPILER_ARGS} )


add_library( libtitanium_config INTERFACE )
target_include_directories( libtitanium_config INTERFACE
    ${CMAKE_SOURCE_DIR}
)
target_sources( libtitanium_config INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/config/config.cpp
)
target_link_libraries( libtitanium_config INTERFACE libtitanium_util )
target_compile_options( libtitanium_config INTERFACE ${TITANIUM_COMPILER_ARGS} )


add_library( libtitanium_logger INTERFACE )
target_include_directories( libtitanium_logger INTERFACE
    ${CMAKE_SOURCE_DIR}
)
target_sources( libtitanium_logger INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/logger/logger.cpp
)
target_compile_options( libtitanium_logger INTERFACE ${TITANIUM_COMPILER_ARGS} )


add_library( libtitanium_sys INTERFACE )
target_include_directories( libtitanium_sys INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
)
target_sources( libtitanium_sys INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/sys/platform_sdl.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/sys/sdl_stringify.cpp
)
target_link_libraries( libtitanium_sys INTERFACE libtitanium_util libtitanium_logger ${SDL2_LIBRARIES} )
target_compile_options( libtitanium_sys INTERFACE ${TITANIUM_COMPILER_ARGS} )


add_library( libtitanium_renderer INTERFACE )
target_include_directories( libtitanium_renderer INTERFACE
    ${CMAKE_SOURCE_DIR}
)
target_sources( libtitanium_renderer INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/renderer/renderer.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/renderer/renderer_device.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/renderer/renderer_resource.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/renderer/renderer_object.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/renderer/renderer_stringify.cpp
)
target_link_libraries( libtitanium_renderer INTERFACE libtitanium_util libtitanium_config libtitanium_logger imgui )
target_compile_options( libtitanium_renderer INTERFACE ${TITANIUM_COMPILER_ARGS} )


add_library( libtitanium_imguiwidgets INTERFACE )
target_include_directories( libtitanium_imguiwidgets INTERFACE
    ${CMAKE_SOURCE_DIR}
)
target_sources( libtitanium_imguiwidgets INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/imgui_widgets/console.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/imgui_widgets/resourceprofiler.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/imgui_widgets/debugoverlay.cpp
)
target_link_libraries( libtitanium_imguiwidgets INTERFACE imgui libtitanium_util libtitanium_config )
target_compile_options( libtitanium_imguiwidgets INTERFACE ${TITANIUM_COMPILER_ARGS} )


add_library( libtitanium_input INTERFACE )
target_include_directories( libtitanium_input INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
)
target_sources( libtitanium_input INTERFACE
    ${CMAKE_SOURCE_DIR}/libtitanium/input/input_device.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/input/input_device_enum.cpp
    ${CMAKE_SOURCE_DIR}/libtitanium/input/input_actions.cpp
)
target_link_libraries( libtitanium_input INTERFACE ${SDL2_LIBRARIES} )
target_compile_options( libtitanium_input INTERFACE ${TITANIUM_COMPILER_ARGS} )



# TODO: VERY TEMP! this should be done in a fancy way of some kind
add_executable( titanium_bin game/game_main.cpp game/game_consolecommand.cpp game/game_loadassimp.cpp )
target_include_directories( titanium_bin PUBLIC ${CMAKE_SOURCE_DIR}/libtitanium ${SDL2_INCLUDE_DIRS} )
target_link_libraries( titanium_bin PUBLIC webgpu assimp openal ${SDL2_LIBRARIES} imgui
    libtitanium_util
    libtitanium_memory
    libtitanium_dev
    libtitanium_config
    libtitanium_logger
    libtitanium_sys
    libtitanium_renderer
    libtitanium_imguiwidgets
    libtitanium_input
)
target_compile_options( titanium_bin INTERFACE ${TITANIUM_COMPILER_ARGS} )

install(TARGETS titanium_bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
