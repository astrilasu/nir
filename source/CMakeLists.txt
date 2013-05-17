cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

set (CMAKE_BUILD_TYPE DEBUG)

#find_package(Boost)
#if (boost_found)
#	include_directories(${BOOST_INCLUDE_DIR})
#	add_definitions("-dhas_boost")
#endif()

set (CMAKE_BINARY_DIR  $(CMAKE_SOURCE_DIR)/../build)
set (CMAKE_CXX_COMPILER /usr/bin/clang++)

include_directories (/usr/include/xercesc /usr/local/include /usr/local/cfitsio/include ./source .)
link_directories (/usr/local/cfitsio/lib /usr/local/lib)

add_executable (test_serial test_serial.cpp serial.cpp serial.h)
add_executable (test_filterwheel test_filterwheel.cpp filterwheel.cpp filterwheel.h
    filter_wheel_manager.h filter_wheel_manager.cpp config_parser.hpp serial.cpp)
add_executable (test_camera test_camera.cpp camera.cpp)
add_executable (flat_field flat_field.cpp camera.cpp logger.cpp)
add_executable (test_exposure test_exposure.cpp camera.cpp)
add_executable (test_fits test_fits.cpp)
add_executable (average_intensity average_intensity.cpp)
add_executable (test_trigger_mode test_trigger_mode.cpp camera.cpp)
add_executable (test_stereo test_stereo.cpp camera.cpp)
add_executable (auto_exposure auto_exposure.cpp camera.cpp camera.h)
add_executable (nir_main serial.cpp filterwheel.cpp camera.cpp main.cpp filter_wheel_manager.cpp logger.cpp)


target_link_libraries (test_filterwheel xerces-c)
target_link_libraries (test_fits cfitsio)
target_link_libraries (average_intensity opencv_highgui opencv_imgproc opencv_core)
target_link_libraries (test_camera ueye_api cfitsio)
target_link_libraries (flat_field ueye_api cfitsio)
target_link_libraries (test_exposure ueye_api cfitsio)
target_link_libraries (test_trigger_mode ueye_api cfitsio)
target_link_libraries (test_stereo ueye_api cfitsio)
target_link_libraries (auto_exposure ueye_api cfitsio)
target_link_libraries (nir_main ueye_api cfitsio xerces-c)
