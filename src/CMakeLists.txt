add_library(bayan_lib
    parser.cpp
    scanner.cpp
    duplicate_finder.cpp
    comparator.cpp
    hasher.cpp
    block_cache.cpp
    filter.cpp
    utilities.cpp
)

target_include_directories(bayan_lib
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

add_executable(bayan
    main.cpp
)

target_link_libraries(bayan PRIVATE
    bayan_lib
    Boost::filesystem
    Boost::system
    Boost::regex
    Boost::program_options
)

set_target_properties(bayan PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

set_target_properties(bayan PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})