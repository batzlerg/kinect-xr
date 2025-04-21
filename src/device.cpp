/**
 * @file device.cpp
 * @brief Implementation of Kinect device management
 */

#include "kinect_xr/device.h"

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

}  // namespace kinect_xr
