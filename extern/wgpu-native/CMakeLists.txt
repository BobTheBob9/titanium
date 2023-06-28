# custom CMakeLists file for wgpu-native

add_library( wgpu-native STATIC IMPORTED GLOBAL )
set_target_properties( wgpu-native PROPERTIES 
	IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libwgpu_native.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}"
)
