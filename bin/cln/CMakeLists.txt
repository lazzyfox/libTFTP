cmake_minimum_required(VERSION 3.2)
project(tftp_cln VERSION 0.0.1 DESCRIPTION "TFTP client test application build")
set(CMAKE_CXX_FLAGS "-O2 -pipe -std=c++2a -pthread -std=gnu++2a -Wall -Wextra -fPIC")
add_executable(tftp_cln ${PROJECT_SOURCE_DIR}/cln.cpp ${PROJECT_SOURCE_DIR}/../../src/libTFTP.hpp)
target_link_libraries(tftp_cln -lgcc -lstdc++ -ldl -lpthread)

