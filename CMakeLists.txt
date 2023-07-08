cmake_minimum_required(VERSION 3.5)

project(titanium LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY _SRCS)

macro (add_sources)
	get_property(SRCS GLOBAL PROPERTY _SRCS)

	file (RELATIVE_PATH _relPath "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
	foreach (_src ${ARGN})
		if (_relPath)
			list (APPEND SRCS "${_relPath}/${_src}")
		else()
			list (APPEND SRCS "${_src}")
		endif()
	endforeach()

	set_property(GLOBAL PROPERTY _SRCS ${SRCS})
endmacro()

include_directories(.)

#find_package( SDL2 REQUIRED ) 

set( WEBGPU_BACKEND WGPU )
#set( DAWN_USE_WAYLAND ON )
add_subdirectory( extern/WebGPU-distribution )
add_subdirectory( extern/imgui )
add_subdirectory( extern/SDL2 )

# core subdirectories
add_subdirectory( titanium )

get_property(SRCS GLOBAL PROPERTY _SRCS)
message(${SRCS})

# TODO: VERY TEMP! this should be done in a fancy way of some kind
add_executable( titanium_bin titanium/programs/engine/engine_main.cpp ${SRCS} )

include_directories( titanium_bin )
target_link_libraries( titanium_bin webgpu SDL2::SDL2 )

set_target_properties(titanium_bin PROPERTIES
	VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1"
)

install(TARGETS titanium_bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
