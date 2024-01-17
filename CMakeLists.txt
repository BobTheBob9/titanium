cmake_minimum_required( VERSION 3.5 )
project( titanium LANGUAGES CXX )

option( TITANIUM_WARNINGS_AS_ERRORS OFF )
if ( TITANIUM_WARNINGS_AS_ERRORS )
    set( TITANIUM_COMPILER_ARGS -Wall -Wextra -Wpedantic -Werror )
else()
    set( TITANIUM_COMPILER_ARGS -Wall -Wextra -Wpedantic ) #-fsanitize=undefined -fsanitize=address
endif()

#set( CMAKE_EXE_LINKER_FLAGS -fsanitize=undefined -fsanitize=address )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

add_compile_definitions( TITANIUM_SDL=1 )
find_package( SDL2 REQUIRED )

find_package( OpenAL REQUIRED )

# statically link assimp
option( ASSIMP_BUILD_ZLIB ON )
option( BUILD_SHARED_LIBS OFF )
find_package( assimp REQUIRED )

add_subdirectory( extern/imgui )
include_directories( INTERFACE ${CMAKE_SOURCE_DIR}/extern )

message( LIBTITANIUM=$ENV{LIBTITANIUM} )
add_subdirectory( $ENV{LIBTITANIUM} build )

add_executable( titanium_bin game/game_main.cpp game/game_consolecommand.cpp  ) #game/game_loadassimp.cpp
target_include_directories( titanium_bin PUBLIC $ENV{LIBTITANIUM} ${SDL2_INCLUDE_DIRS} )
target_link_libraries( titanium_bin PUBLIC assimp openal ${SDL2_LIBRARIES} imgui
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
