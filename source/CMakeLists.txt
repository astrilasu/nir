cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

set (CMAKE_BUILD_TYPE DEBUG)

#find_package(Boost)
#if (boost_found)
#	include_directories(${BOOST_INCLUDE_DIR})
#	add_definitions("-dhas_boost")
#endif()

set (CMAKE_BINARY_DIR  $(CMAKE_SOURCE_DIR)/../build)

include_directories (/usr/include/xercesc /usr/local/cfitsio/include ./source .)
link_directories (/usr/local/cfitsio/lib)

add_executable (test_serial test_serial.cpp serial.cpp serial.h)
add_executable (test_filter_wheel test_filterwheel.cpp filter_wheel.cpp config_parser.hpp serial.cpp)
add_executable (test_camera test_camera.cpp camera.cpp)
add_executable (nir_main serial.cpp filter_wheel.cpp camera.cpp main.cpp)


target_link_libraries (test_filter_wheel xerces-c)
target_link_libraries (test_camera ueye_api)
target_link_libraries (nir_main ueye_api cfitsio xerces-c)
