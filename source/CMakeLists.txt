cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

set (CMAKE_BUILD_TYPE DEBUG)

#find_package(Boost)
#if (boost_found)
#	include_directories(${BOOST_INCLUDE_DIR})
#	add_definitions("-dhas_boost")
#endif()

set (CMAKE_BINARY_DIR  $(CMAKE_SOURCE_DIR)/../build)

add_executable (test_serial test_serial.cpp serial.cpp serial.h)
