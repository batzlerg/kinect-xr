add_executable(unit_tests
  unit_test_main.cpp
  libfreenect_test.cpp
  device_unit_test.cpp
)

target_link_libraries(unit_tests
  PRIVATE
  gtest
  gtest_main
  kinect_xr_device
)

target_include_directories(unit_tests
  PRIVATE
  ${LIBFREENECT_INCLUDE_DIR}
)

add_test(
  NAME UnitTests
  COMMAND unit_tests
)

set_tests_properties(UnitTests PROPERTIES LABELS "unit")
