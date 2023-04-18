cmake_minimum_required(VERSION 3.0.0)
project(libtftp_tests VERSION 0.0.2)
set(CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_COMPILER_VERSION 12)
set(CMAKE_CXX_COMPILER "/usr/bin/g++-12")
set(CMAKE_C_COMPILER ${C_COMPILER})
set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -Wextra -fPIC")
# Find additional packages
find_package(GTest CONFIG REQUIRED)
# Creating tests
include(CTest)
enable_testing()
find_package(GTest CONFIG REQUIRED)
add_test(NAME TestTFTP COMMAND libtftp_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build/tests)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
# Finial test
add_executable(libtftp_tests tests.cpp)
target_link_libraries(libtftp_tests PRIVATE GTest::gtest)
# Run unit tests after compilation
#include(GoogleTest)
#gtest_discover_tests(libtftp_tests)
