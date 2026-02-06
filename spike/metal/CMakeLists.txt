cmake_minimum_required(VERSION 3.20)
project(MetalKinectViewer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find libfreenect
find_library(LIBFREENECT_LIBRARY freenect)
find_path(LIBFREENECT_INCLUDE_DIR libfreenect/libfreenect.h)

if(NOT LIBFREENECT_LIBRARY OR NOT LIBFREENECT_INCLUDE_DIR)
  message(FATAL_ERROR "libfreenect not found. Please install it first.")
endif()

# Import kinect_xr_device library from parent build
set(KINECT_XR_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
find_library(KINECT_XR_DEVICE_LIB
    NAMES kinect_xr_device
    PATHS ${KINECT_XR_ROOT}/build/lib
    NO_DEFAULT_PATH
)

if(NOT KINECT_XR_DEVICE_LIB)
    message(FATAL_ERROR "kinect_xr_device library not found. Please build the main project first: cd ${KINECT_XR_ROOT} && cmake --build build")
endif()

message(STATUS "Found kinect_xr_device: ${KINECT_XR_DEVICE_LIB}")

# Metal shader compilation
set(METAL_SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/MetalKinectViewer/Shaders.metal")
set(METAL_LIBRARY_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/default.metallib")

add_custom_command(
    OUTPUT ${METAL_LIBRARY_OUTPUT}
    COMMAND xcrun -sdk macosx metal -c ${METAL_SHADER_SOURCE} -o ${CMAKE_CURRENT_BINARY_DIR}/Shaders.air
    COMMAND xcrun -sdk macosx metallib ${CMAKE_CURRENT_BINARY_DIR}/Shaders.air -o ${METAL_LIBRARY_OUTPUT}
    DEPENDS ${METAL_SHADER_SOURCE}
    COMMENT "Compiling Metal shaders"
)

add_custom_target(shaders ALL DEPENDS ${METAL_LIBRARY_OUTPUT})

# Executable
add_executable(MetalKinectViewer
    MetalKinectViewer/main.mm
)

target_compile_options(MetalKinectViewer PRIVATE
    -fobjc-arc  # Enable ARC
)

target_include_directories(MetalKinectViewer PRIVATE
    ${KINECT_XR_ROOT}/include
    ${LIBFREENECT_INCLUDE_DIR}
)

target_link_libraries(MetalKinectViewer
    "-framework Cocoa"
    "-framework Metal"
    "-framework MetalKit"
    "-framework QuartzCore"
    ${KINECT_XR_DEVICE_LIB}
    ${LIBFREENECT_LIBRARY}
)

# Ensure shaders are built before executable
add_dependencies(MetalKinectViewer shaders)

# Copy metallib to binary directory for runtime loading
add_custom_command(TARGET MetalKinectViewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${METAL_LIBRARY_OUTPUT}
        $<TARGET_FILE_DIR:MetalKinectViewer>/default.metallib
    COMMENT "Copying Metal library to executable directory"
)
