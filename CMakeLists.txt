cmake_minimum_required(VERSION 3.28)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION 0.0.${PATCH_VERSION})

project(bayan VERSION ${PROJECT_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

enable_testing()

find_package(Boost REQUIRED COMPONENTS
    filesystem
    system
    regex
    program_options
)

add_subdirectory(src)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_VENDOR "oflika256")
set(CPACK_PACKAGE_CONTACT "oflika256@gmail.com")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH ${PATCH_VERSION})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "oflika256 <oflika256@gmail.com")

include(CPack)
