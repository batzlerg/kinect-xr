/**
 * @file libfreenect_test.cpp
 * @brief Unit tests for basic libfreenect functionality
 *
 * These tests verify that we can properly initialize libfreenect and
 * perform basic operations that don't require physical hardware.
 *
 * We don't want to test the whole lib, but having some validation that it's installed
 * and working will help guard against API changes or misconfigurations.
 */

#include <gtest/gtest.h>
#include <libfreenect/libfreenect.h>

#include <iostream>

/**
 * @brief Test suite for basic libfreenect functionality
 */
class LibfreenectTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // initialized to test creation only
    ctx = nullptr;
  }

  void TearDown() override {
    if (ctx) {
      freenect_shutdown(ctx);
      ctx = nullptr;
    }
  }

  freenect_context* ctx;
};

/**
 * @brief Test libfreenect context initialization
 *
 * This test verifies that the libfreenect library is available and that
 * we can successfully initialize a context, no physical Kinect device required
 */
TEST_F(LibfreenectTest, ContextInitialization) {
  int result = freenect_init(&ctx, nullptr);

  EXPECT_EQ(result, 0) << "Failed to initialize libfreenect context";
  EXPECT_NE(ctx, nullptr) << "libfreenect context is null after initialization";
}

/**
 * @brief Test device enumeration
 *
 * Verifies that we can query for connected Kinect devices.
 * This test doesn't assert what the count is, since that
 * depends on the test environment, but ensures the API works correctly.
 */
TEST_F(LibfreenectTest, DeviceEnumeration) {
  if (freenect_init(&ctx, nullptr) != 0) {
    GTEST_SKIP() << "Could not initialize libfreenect context, skipping test";
  }

  int numDevices = freenect_num_devices(ctx);

  // we don't check for a specific number since it depends on the system
  // but we output it as a debugging aid
  std::cout << "Found " << numDevices << " Kinect device(s)" << std::endl;

  // verify the api doesn't return a negative number
  EXPECT_GE(numDevices, 0) << "Device count should not be negative";
}

/**
 * @brief Test subdevice selection
 *
 * Verifies that we can set subdevice flags without errors.
 * This doesn't require physical hardware.
 */
TEST_F(LibfreenectTest, SubdeviceSelection) {
  // only run this test if we can initialize the context
  if (freenect_init(&ctx, nullptr) != 0) {
    GTEST_SKIP() << "Could not initialize libfreenect context, skipping test";
  }

  // test setting different subdevice combinations
  freenect_select_subdevices(ctx, FREENECT_DEVICE_MOTOR);
  freenect_select_subdevices(ctx, FREENECT_DEVICE_CAMERA);
  freenect_select_subdevices(ctx, FREENECT_DEVICE_AUDIO);
  freenect_select_subdevices(
      ctx, static_cast<freenect_device_flags>(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA));
  freenect_select_subdevices(
      ctx, static_cast<freenect_device_flags>(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA |
                                              FREENECT_DEVICE_AUDIO));

  // if we got here without crashing, the test passes
  SUCCEED();
}

/**
 * @brief Test video and depth mode enumeration
 *
 * Verifies that we can query for supported video and depth modes.
 * This doesn't require physical hardware.
 */
TEST_F(LibfreenectTest, ModeEnumeration) {
  freenect_frame_mode mode;

  // medium resolution rgb
  mode = freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB);
  EXPECT_TRUE(mode.is_valid);
  EXPECT_EQ(mode.width, 640);
  EXPECT_EQ(mode.height, 480);

  // depth modes
  mode = freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT);
  EXPECT_TRUE(mode.is_valid);
  EXPECT_EQ(mode.width, 640);
  EXPECT_EQ(mode.height, 480);
}
