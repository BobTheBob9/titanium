# sys contains various helpers for interfacing with platform-specific stuff in a generic way
include_directories( platform/sdl/* )
add_sources(
    platform/sdl/sys_sdl.cpp
)