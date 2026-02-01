add_executable(bulk_tests
    
)

target_include_directories(bulk_tests
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

find_package(GTest REQUIRED)

target_link_libraries(bulk_tests
    bulk_core
    GTest::gtest_main
)

add_test(NAME bulk_tests COMMAND bulk_tests)