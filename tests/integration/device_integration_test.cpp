#include <gtest/gtest.h>
#include <libfreenect/libfreenect.h>

#include <memory>

#include "kinect_xr/device.h"

using namespace kinect_xr;

/**
 * @brief Test fixture for Kinect device testing
 */
class KinectDeviceTest : public ::testing::Test {
 protected:
  void SetUp() override { device = std::make_unique<KinectDevice>(); }

  void TearDown() override { device.reset(); }

  std::unique_ptr<KinectDevice> device;
};

/**
 * @brief Test that verifies if Kinect hardware is properly detected
 */
TEST(KinectHardwareTest, DetectKinectDevice) {
  // initialize
  freenect_context* ctx = nullptr;
  ASSERT_EQ(freenect_init(&ctx, nullptr), 0) << "Failed to initialize freenect context";

  // check for connected devices
  int num_devices = freenect_num_devices(ctx);
  std::cout << "Found " << num_devices << " Kinect device(s)" << std::endl;

  EXPECT_GT(num_devices, 0) << "No Kinect devices found, but should have been detected";

  // clean up
  freenect_shutdown(ctx);
}

/**
 * @brief Test that tries to open the first Kinect device
 */
TEST(KinectHardwareTest, OpenKinectDevice) {
  // initialize freenect
  freenect_context* ctx = nullptr;
  ASSERT_EQ(freenect_init(&ctx, nullptr), 0) << "Failed to initialize freenect context";

  // open first device
  freenect_device* dev = nullptr;
  int result = freenect_open_device(ctx, &dev, 0);
  EXPECT_EQ(result, 0) << "Failed to open Kinect device";

  // clean up
  if (dev) {
    freenect_close_device(dev);
  }
  freenect_shutdown(ctx);
}

/**
 * @brief Test device initialization
 */
TEST_F(KinectDeviceTest, Initialize) {
  DeviceConfig config;
  config.enableRGB = true;
  config.enableDepth = true;

  DeviceError result = device->initialize(config);

  EXPECT_EQ(result, DeviceError::None);
  EXPECT_TRUE(device->isInitialized());
}