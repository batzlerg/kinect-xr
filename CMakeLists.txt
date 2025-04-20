cmake_minimum_required(VERSION 4.0)

project(KinectXR VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_executable(kinect_test src/kinect_test.cpp)

# libfreenect
find_library(LIBFREENECT_LIBRARY freenect)
find_path(LIBFREENECT_INCLUDE_DIR libfreenect/libfreenect.h)

if(NOT LIBFREENECT_LIBRARY OR NOT LIBFREENECT_INCLUDE_DIR)
    message(FATAL_ERROR "libfreenect not found. Please install it first.")
endif()

message(STATUS "Found libfreenect: ${LIBFREENECT_LIBRARY}")

# targets
target_include_directories(kinect_test PRIVATE ${LIBFREENECT_INCLUDE_DIR})
target_link_libraries(kinect_test PRIVATE ${LIBFREENECT_LIBRARY})