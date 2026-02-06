add_executable(openxr_spike_test openxr_sdk_test.cpp)

target_link_libraries(openxr_spike_test
  PRIVATE
  openxr_loader
)
