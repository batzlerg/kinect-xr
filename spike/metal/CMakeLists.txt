cmake_minimum_required(VERSION 3.20)
project(MetalKinectViewer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

target_link_libraries(MetalKinectViewer
    "-framework Cocoa"
    "-framework Metal"
    "-framework MetalKit"
    "-framework QuartzCore"
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
