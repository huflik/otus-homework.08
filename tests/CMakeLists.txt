enable_testing()

add_executable(bayan_tests   
   test_hasher.cpp
   test_filter.cpp
   test_block_cache.cpp
   test_comparator.cpp
   test_duplicate_finder.cpp
   test_scanner.cpp

)

target_include_directories(bayan_tests
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(bayan_tests
    bayan_lib
    GTest::gtest
    GTest::gtest_main
    Boost::filesystem
    Boost::system
    Boost::regex
    Boost::program_options
)

add_test(NAME bayan_tests COMMAND bayan_tests)

   #test_hasher.cpp
   #test_filter.cpp
   #test_block_cache.cpp
   #test_comparator.cpp
   #test_duplicate_finder.cpp
   #test_scanner.cpp