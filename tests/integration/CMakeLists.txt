add_executable(integration_tests
  integration_test_main.cpp
  device_integration_test.cpp
)

target_link_libraries(integration_tests
  PRIVATE
  gtest
  gtest_main
  ${LIBFREENECT_LIBRARY}
  kinect_xr_device
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
