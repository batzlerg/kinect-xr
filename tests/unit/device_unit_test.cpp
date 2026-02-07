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
  EXPECT_EQ(errorToString(DeviceError::MotorControlFailed), "Motor control failed");
  EXPECT_EQ(errorToString(DeviceError::InvalidParameter), "Invalid parameter");
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

// Motor Control Unit Tests (M3)

TEST(KinectDeviceMotorUnitTest, MotorControlFailsWhenNotInitialized) {
  KinectDevice device;
  EXPECT_FALSE(device.isInitialized());

  // Test setTiltAngle
  DeviceError result = device.setTiltAngle(0.0);
  EXPECT_EQ(result, DeviceError::NotInitialized);

  // Test getTiltAngle
  double angle = 0.0;
  result = device.getTiltAngle(angle);
  EXPECT_EQ(result, DeviceError::NotInitialized);

  // Test setLED
  result = device.setLED(LEDState::Green);
  EXPECT_EQ(result, DeviceError::NotInitialized);

  // Test getMotorStatus
  MotorStatus status;
  result = device.getMotorStatus(status);
  EXPECT_EQ(result, DeviceError::NotInitialized);
}

// Motor tests with hardware (if available)
class KinectDeviceMotorTest : public ::testing::Test {
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

  void TearDown() override {
    // Reset motor to 0 degrees if hardware present
    if (hasHardware && device) {
      device->setTiltAngle(0.0);
    }
    device.reset();
  }

  std::unique_ptr<KinectDevice> device;
  bool hasHardware = false;
};

TEST_F(KinectDeviceMotorTest, SetTiltAngleWithinRange) {
  if (!hasHardware) {
    GTEST_SKIP() << "No hardware available";
  }

  // Test setting angle within valid range
  DeviceError result = device->setTiltAngle(10.0);
  EXPECT_EQ(result, DeviceError::None);

  // Verify angle was set (allow for motor precision tolerance)
  double angle = 0.0;
  result = device->getTiltAngle(angle);
  EXPECT_EQ(result, DeviceError::None);
  // Motor may still be moving or have precision limits
  EXPECT_NEAR(angle, 10.0, 5.0);
}

TEST_F(KinectDeviceMotorTest, SetTiltAngleClampsToLimits) {
  if (!hasHardware) {
    GTEST_SKIP() << "No hardware available";
  }

  // Test upper limit clamping (should clamp to +27)
  DeviceError result = device->setTiltAngle(50.0);
  EXPECT_EQ(result, DeviceError::None);

  // Test lower limit clamping (should clamp to -27)
  result = device->setTiltAngle(-50.0);
  EXPECT_EQ(result, DeviceError::None);
}

TEST_F(KinectDeviceMotorTest, SetLEDStates) {
  if (!hasHardware) {
    GTEST_SKIP() << "No hardware available";
  }

  // Test all LED states
  EXPECT_EQ(device->setLED(LEDState::Off), DeviceError::None);
  EXPECT_EQ(device->setLED(LEDState::Green), DeviceError::None);
  EXPECT_EQ(device->setLED(LEDState::Red), DeviceError::None);
  EXPECT_EQ(device->setLED(LEDState::Yellow), DeviceError::None);
  EXPECT_EQ(device->setLED(LEDState::BlinkGreen), DeviceError::None);
  EXPECT_EQ(device->setLED(LEDState::BlinkRedYellow), DeviceError::None);

  // Reset to off
  EXPECT_EQ(device->setLED(LEDState::Off), DeviceError::None);
}

TEST_F(KinectDeviceMotorTest, GetMotorStatus) {
  if (!hasHardware) {
    GTEST_SKIP() << "No hardware available";
  }

  MotorStatus status;
  DeviceError result = device->getMotorStatus(status);
  EXPECT_EQ(result, DeviceError::None);

  // Validate status structure is populated
  // Tilt angle should be within hardware limits
  EXPECT_GE(status.tiltAngle, -27.0);
  EXPECT_LE(status.tiltAngle, 27.0);

  // Accelerometer values should be non-zero (gravity always present)
  // At least one axis should show ~9.8 m/s² (1g)
  double totalAccel = std::abs(status.accelX) + std::abs(status.accelY) + std::abs(status.accelZ);
  EXPECT_GT(totalAccel, 5.0);  // Reasonable threshold for gravity detection
}
