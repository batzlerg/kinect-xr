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
