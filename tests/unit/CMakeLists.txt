add_executable(unit_tests
  unit_test_main.cpp
  libfreenect_test.cpp
  device_unit_test.cpp
  system_management_test.cpp
  session_management_test.cpp
  swapchain_test.cpp
  frame_loop_test.cpp
  frame_cache_test.cpp
  texture_upload_test.cpp
)

target_link_libraries(unit_tests
  PRIVATE
  gtest
  gtest_main
  kinect_xr_device
  kinect_xr_runtime_lib
  OpenXR::openxr_loader
)

target_include_directories(unit_tests
  PRIVATE
  ${LIBFREENECT_INCLUDE_DIR}
)

target_compile_definitions(unit_tests
  PRIVATE
  XR_USE_GRAPHICS_API_METAL
)

add_test(
  NAME UnitTests
  COMMAND unit_tests
)

set_tests_properties(UnitTests PROPERTIES LABELS "unit")
