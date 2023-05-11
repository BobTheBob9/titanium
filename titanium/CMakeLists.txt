# titanium contains the main source for the titanium engine
add_subdirectory( memory )
add_subdirectory( logger )
add_subdirectory( util )
add_subdirectory( devmode )
add_subdirectory( config )
add_subdirectory( filesystem )

add_subdirectory( renderer )

include_directories(*)