#include <gtest/gtest.h>

#include "kinect_xr/device.h"

using namespace kinect_xr;

TEST(KinectDeviceUnitTest, ErrorToString) {
  EXPECT_EQ(errorToString(DeviceError::None), "No error");
  EXPECT_EQ(errorToString(DeviceError::DeviceNotFound), "Device not found");
  EXPECT_EQ(errorToString(DeviceError::InitializationFailed), "Initialization failed");
  EXPECT_EQ(errorToString(DeviceError::Unknown), "Unknown error");
}

TEST(KinectDeviceUnitTest, ConstructionDestruction) { KinectDevice device; }

TEST(KinectDeviceUnitTest, DeviceCount) {
  int count = KinectDevice::getDeviceCount();

  // we don't know how many, but the count shouldn't be negative
  EXPECT_GE(count, 0);

  std::cout << "Found " << count << " Kinect device(s)" << std::endl;
}

class KinectDeviceInitTest : public ::testing::Test {
 protected:
  void SetUp() override { device = std::make_unique<KinectDevice>(); }

  void TearDown() override { device.reset(); }

  std::unique_ptr<KinectDevice> device;
};

// test initialization without hardware
TEST_F(KinectDeviceInitTest, InitializationStatus) {
  EXPECT_FALSE(device->isInitialized());

  // return appropriate error
  if (KinectDevice::getDeviceCount() == 0) {
    DeviceConfig config;
    DeviceError result = device->initialize(config);
    EXPECT_EQ(result, DeviceError::DeviceNotFound);
    EXPECT_FALSE(device->isInitialized());
  }
}
