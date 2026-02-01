add_library(bayan_core
    block_cache.cpp
    comparer.cpp
    file_reader.cpp
    hasher.cpp
    scanner.cpp
    filter.cpp
)

target_include_directories(bayan_core
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

add_executable(bayan main.cpp)

target_link_libraries(bayan
    bayan_core
)