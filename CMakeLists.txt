cmake_minimum_required(VERSION 3.2)
project(tftp_srv VERSION 0.0.3 DESCRIPTION "TFTP server library")
set(CMAKE_CXX_FLAGS "-g -pipe -std=c++2a -pthread -g -std=gnu++2a -Wall -Wextra -fPIC")
add_executable(tftp_srv /home/fox/fast_proj/libTFTP/src/main.cpp /home/fox/fast_proj/libTFTP/src/libTFTP.hpp)
target_link_libraries(tftp_srv -lgcc -lstdc++ -ldl -lpthread)

