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

# core subdirectories
add_subdirectory(titanium)

get_property(SRCS GLOBAL PROPERTY _SRCS)
message(${SRCS})

find_package( Vulkan REQUIRED )
find_package( glfw3 REQUIRED )

# TODO: VERY TEMP! this should be done in a fancy way of some kind
add_executable(titanium_bin titanium/programs/engine/engine_main.cpp ${SRCS})

# TODO: TEMP
target_include_directories (titanium_bin 
    PUBLIC ${GLFW3_INCLUDE_DIRS}
    PUBLIC ${VULKAN_INCLUDE_DIRS}
)

target_link_libraries (titanium_bin vulkan ${VULKAN_LIBRARIES})
target_link_libraries (titanium_bin glfw ${GLFW_LIBRARIES})

install(TARGETS titanium_bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
