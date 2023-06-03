cmake_minimum_required(VERSION 3.0.0)
project(libtftp_tests VERSION 0.0.6)
set(CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_COMPILER_VERSION 13)
set(CMAKE_C_COMPILER ${C_COMPILER})
set (CMAKE_CXX_COMPILER ${CXX_COMPILER})
#set(CMAKE_CXX_COMPILER "/usr/bin/gcc-13")
#set(CMAKE_C_COMPILER ${C_COMPILER})
set(CMAKE_CXX_FLAGS "-ggdb -std=c++2b -Wall -Wextra -fPIC")
# Find additional packages
find_package(GTest CONFIG REQUIRED)
# Creating unit tests
add_executable(libtftp_tests tests.cpp)
target_link_libraries(libtftp_tests PRIVATE GTest::gtest)
# Run unit tests after compilation
#include(GoogleTest)
#gtest_discover_tests(libtftp_tests)
