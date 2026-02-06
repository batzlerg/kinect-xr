add_executable(integration_tests
  integration_test_main.cpp
  device_integration_test.cpp
  runtime_discovery_test.cpp
)

target_link_libraries(integration_tests
  PRIVATE
  gtest
  gtest_main
  ${LIBFREENECT_LIBRARY}
  kinect_xr_device
  openxr_loader
)

target_include_directories(integration_tests
  PRIVATE
  ${LIBFREENECT_INCLUDE_DIR}
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
