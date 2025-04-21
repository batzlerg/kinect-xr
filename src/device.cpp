/**
 * @file device.cpp
 * @brief Implementation of Kinect device management
 */

#include "kinect_xr/device.h"

#include <libfreenect/libfreenect.h>

namespace kinect_xr {

std::string errorToString(DeviceError error) {
  switch (error) {
    case DeviceError::None:
      return "No error";
    case DeviceError::DeviceNotFound:
      return "Device not found";
    case DeviceError::InitializationFailed:
      return "Initialization failed";
    case DeviceError::Unknown:
    default:
      return "Unknown error";
  }
}

KinectDevice::KinectDevice() {}

KinectDevice::~KinectDevice() {}

DeviceError KinectDevice::initialize(const DeviceConfig& config) {
  if (initialized_) {
    return DeviceError::None;
  }

  // store configuration
  config_ = config;

  if (getDeviceCount() == 0) {
    return DeviceError::DeviceNotFound;
  }

  // todo: actually initialize
  // for now, just mark as initialized
  initialized_ = true;
  return DeviceError::None;
}

bool KinectDevice::isInitialized() const { return initialized_; }

int KinectDevice::getDeviceCount() {
  freenect_context* ctx = nullptr;
  if (freenect_init(&ctx, nullptr) < 0) {
    return 0;
  }

  int count = freenect_num_devices(ctx);
  freenect_shutdown(ctx);

  return count;
}

}  // namespace kinect_xr
