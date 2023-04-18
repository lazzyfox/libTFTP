cmake_minimum_required(VERSION 3.2)
project(tftp_srv VERSION 0.0.1 DESCRIPTION "TFTP server test application build")
set(CMAKE_CXX_FLAGS "-O2 -pipe -std=c++2a -pthread -std=gnu++2a -Wall -Wextra -fPIC")
add_executable(tftp_srv ${PROJECT_SOURCE_DIR}/srv.cpp ${PROJECT_SOURCE_DIR}/../../src/libTFTP.hpp)
target_link_libraries(tftp_srv -lgcc -lstdc++ -ldl -lpthread)

