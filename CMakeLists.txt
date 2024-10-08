cmake_minimum_required(VERSION 3.9)
project(libTFTP VERSION 0.0.25 DESCRIPTION "TFTP server library")

IF (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  message (FATAL_ERROR "Not a linux sytem. Processing terminated")
ENDIF (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux")

option(BUILD_ALL "Build TESTING, BINARY FILES, and DOCUMENTATION BUILD_ALL is enabled." OFF)
option(BUILD_TESTS "Build the unit tests when BUILD_TESTING is enabled." OFF)
option(BUILD_DOC "Build the unit tests when BUILD_TESTING is enabled." OFF)
option(BUILD_BINARY "Build the bin file for autotesting when BUILD_BINARY is enabled." OFF)

set(DEFAULT_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-o2")
set (CMAKE_CXX_COMPILER_VERSION 13)
set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER ${C_COMPILER})
set (CMAKE_CXX_COMPILER ${CXX_COMPILER})
set(CMAKE_C_FLAGS_DEBUG "-ggdb -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -DDEBUG")
set(CMAKE_CXX_FLAGS "-std=gnu++2b -ggdb")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


include(GNUInstallDirs)


add_library(libTFTP INTERFACE)
target_compile_features(libTFTP INTERFACE cxx_std_23)
#target_sources(clidict INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src/libTFTP.hpp)

install(TARGETS libTFTP
        EXPORT ${PROJECT_NAME}_Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


target_include_directories(libTFTP INTERFACE 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("tftplibConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion)
configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION
  ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)



install(EXPORT ${PROJECT_NAME}_Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
              "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/ DESTINATION include)

if (BUILD_ALL)
  message(STATUS "Creating all")
  set (BUILD_TESTS ON)
  set (BUILD_BINARY ON)
  set (BUILD_DOC ON)
endif (BUILD_ALL)


if (BUILD_TESTS)
  message(STATUS "Making unit tests")
  include(CTest)
  enable_testing()
  find_package(GTest CONFIG REQUIRED)
  add_subdirectory(tests)
  target_include_directories(libtftp_tests PRIVATE tests/include)
endif (BUILD_TESTS)


if (BUILD_BINARY)
  message(STATUS "Making server and client")
  add_subdirectory(bin/srv)
  add_subdirectory(bin/cln)
endif (BUILD_BINARY)


if (BUILD_DOC)
  message(STATUS "Making documentation")
  find_package(Doxygen)
  if (DOXYGEN_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/doc)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    # note the option ALL which allows to build the docs together with the application
    add_custom_target( doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
  else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
  endif (DOXYGEN_FOUND)
endif (BUILD_DOC)

