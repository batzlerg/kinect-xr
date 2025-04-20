#include <gtest/gtest.h>
#include <libfreenect/libfreenect.h>

#include <iostream>

/**
 * @brief Check if Kinect hardware is connected
 * @return true if at least one device is available
 */
bool isKinectConnected() {
  freenect_context* ctx = nullptr;
  if (freenect_init(&ctx, nullptr) < 0) {
    return false;
  }

  int count = freenect_num_devices(ctx);
  freenect_shutdown(ctx);

  return count > 0;
}

/**
 * @brief Custom main for integration tests early hardware check
 *
 * Integration tests require actual hardware.
 * They will be skipped if no device is Kinected.
 */
int main(int argc, char** argv) {
  std::cout << "Kinect XR Integration Tests" << std::endl;

  // no hardware
  if (!isKinectConnected()) {
    std::cout << "No Kinect device connected. Integration tests skipped." << std::endl;
    return 77;  // special return code to tell ctest to skip this test
  }

  // yes hardware
  std::cout << "Kinect hardware detected - proceeding with integration tests" << std::endl;

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}