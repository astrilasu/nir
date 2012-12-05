cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

set (CMAKE_BUILD_TYPE DEBUG)

#find_package(Boost)
#if (boost_found)
#	include_directories(${BOOST_INCLUDE_DIR})
#	add_definitions("-dhas_boost")
#endif()

set (CMAKE_BINARY_DIR  $(CMAKE_SOURCE_DIR)/../build)

include_directories (/usr/include/xercesc ./source .)

add_executable (test_serial test_serial.cpp serial.cpp serial.h)
add_executable (test_filter_wheel test_filterwheel.cpp filter_wheel.cpp config_parser.hpp serial.cpp)


target_link_libraries (test_filter_wheel xerces-c)
