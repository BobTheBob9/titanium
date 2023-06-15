# util contains various standalone utility types, data structures, functions etc
include_directories(*)
add_sources(
    string.cpp
)

# util/data contains core data structures used across titanium
include_directories(data/*)
add_sources(
    data/datanode.cpp
    data/vector_tests.cpp
)

# util/parser contains parsers for various data structures used across titanium
# some are simple (e.g. just return pointers), others are for structures in util/data
include_directories(parser/*)
add_sources(
    parser/bdef.cpp
    parser/namespaced.cpp
)

