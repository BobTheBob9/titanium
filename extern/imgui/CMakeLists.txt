add_library( imgui INTERFACE )
target_include_directories( imgui INTERFACE . )
target_sources( imgui INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_sdl2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_wgpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp
)
