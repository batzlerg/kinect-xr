add_executable(integration_tests
  integration_test_main.cpp
  device_integration_test.cpp
  runtime_discovery_test.cpp
  kinect_integration_test.cpp
)

target_link_libraries(integration_tests
  PRIVATE
  gtest
  gtest_main
  ${LIBFREENECT_LIBRARY}
  kinect_xr_device
  kinect_xr_runtime_lib
  openxr_loader
)

target_include_directories(integration_tests
  PRIVATE
  ${LIBFREENECT_INCLUDE_DIR}
)

target_compile_definitions(integration_tests
  PRIVATE
  XR_USE_GRAPHICS_API_METAL
)

add_test(
  NAME IntegrationTests
  COMMAND integration_tests
)

set_tests_properties(IntegrationTests PROPERTIES LABELS "integration")
set_tests_properties(IntegrationTests PROPERTIES SKIP_RETURN_CODE 77)

# Runtime discovery tests (no hardware required)
add_executable(runtime_tests
  runtime_discovery_test.cpp
  session_lifecycle_test.cpp
)

target_link_libraries(runtime_tests
  PRIVATE
  gtest
  gtest_main
  openxr_loader
)

target_compile_definitions(runtime_tests
  PRIVATE
  XR_USE_GRAPHICS_API_METAL
)

add_test(
  NAME RuntimeTests
  COMMAND runtime_tests
)

set_tests_properties(RuntimeTests PROPERTIES LABELS "runtime")

# Swapchain Metal tests (requires Metal, no Kinect hardware)
add_executable(swapchain_metal_tests
  swapchain_metal_test.mm
)

target_link_libraries(swapchain_metal_tests
  PRIVATE
  gtest
  gtest_main
  kinect_xr_runtime_lib
  OpenXR::openxr_loader
  "-framework Metal"
  "-framework Foundation"
)

target_compile_definitions(swapchain_metal_tests
  PRIVATE
  XR_USE_GRAPHICS_API_METAL
)

add_test(
  NAME SwapchainMetalTests
  COMMAND swapchain_metal_tests
)

set_tests_properties(SwapchainMetalTests PROPERTIES LABELS "integration")
