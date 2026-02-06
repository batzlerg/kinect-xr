#include <gtest/gtest.h>

#include "kinect_xr/device.h"

using namespace kinect_xr;

TEST(KinectDeviceUnitTest, ErrorToString) {
  EXPECT_EQ(errorToString(DeviceError::None), "No error");
  EXPECT_EQ(errorToString(DeviceError::DeviceNotFound), "Device not found");
  EXPECT_EQ(errorToString(DeviceError::InitializationFailed), "Initialization failed");
  EXPECT_EQ(errorToString(DeviceError::NotInitialized), "Device not initialized");
  EXPECT_EQ(errorToString(DeviceError::AlreadyStreaming), "Streams already active");
  EXPECT_EQ(errorToString(DeviceError::NotStreaming), "Streams not active");
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

// M4: Streaming state tests
TEST(KinectDeviceUnitTest, StreamingInitialState) {
  KinectDevice device;
  EXPECT_FALSE(device.isStreaming());
}

TEST(KinectDeviceUnitTest, StartStreamsFailsWhenNotInitialized) {
  KinectDevice device;
  EXPECT_FALSE(device.isInitialized());

  DeviceError result = device.startStreams();
  EXPECT_EQ(result, DeviceError::NotInitialized);
  EXPECT_FALSE(device.isStreaming());
}

TEST(KinectDeviceUnitTest, StopStreamsFailsWhenNotInitialized) {
  KinectDevice device;

  DeviceError result = device.stopStreams();
  EXPECT_EQ(result, DeviceError::NotInitialized);
  EXPECT_FALSE(device.isStreaming());
}

// Test with hardware (if available)
class KinectDeviceStreamTest : public ::testing::Test {
 protected:
  void SetUp() override {
    device = std::make_unique<KinectDevice>();
    hasHardware = KinectDevice::getDeviceCount() > 0;

    if (hasHardware) {
      DeviceConfig config;
      DeviceError result = device->initialize(config);
      ASSERT_EQ(result, DeviceError::None);
      ASSERT_TRUE(device->isInitialized());
    }
  }

  void TearDown() override { device.reset(); }

  std::unique_ptr<KinectDevice> device;
  bool hasHardware = false;
};

TEST_F(KinectDeviceStreamTest, StartStreamsSetsStreamingFlag) {
  if (!hasHardware) {
    GTEST_SKIP() << "No hardware available";
  }

  EXPECT_FALSE(device->isStreaming());

  DeviceError result = device->startStreams();
  EXPECT_EQ(result, DeviceError::None);
  EXPECT_TRUE(device->isStreaming());
}

TEST_F(KinectDeviceStreamTest, StartStreamsFailsWhenAlreadyStreaming) {
  if (!hasHardware) {
    GTEST_SKIP() << "No hardware available";
  }

  DeviceError result = device->startStreams();
  EXPECT_EQ(result, DeviceError::None);
  EXPECT_TRUE(device->isStreaming());

  // Try to start again
  result = device->startStreams();
  EXPECT_EQ(result, DeviceError::AlreadyStreaming);
  EXPECT_TRUE(device->isStreaming());
}

TEST_F(KinectDeviceStreamTest, StopStreamsFailsWhenNotStreaming) {
  if (!hasHardware) {
    GTEST_SKIP() << "No hardware available";
  }

  EXPECT_FALSE(device->isStreaming());

  DeviceError result = device->stopStreams();
  EXPECT_EQ(result, DeviceError::NotStreaming);
  EXPECT_FALSE(device->isStreaming());
}

TEST_F(KinectDeviceStreamTest, StopStreamsClearsStreamingFlag) {
  if (!hasHardware) {
    GTEST_SKIP() << "No hardware available";
  }

  DeviceError result = device->startStreams();
  EXPECT_EQ(result, DeviceError::None);
  EXPECT_TRUE(device->isStreaming());

  result = device->stopStreams();
  EXPECT_EQ(result, DeviceError::None);
  EXPECT_FALSE(device->isStreaming());
}
