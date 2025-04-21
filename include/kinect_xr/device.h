/**
 * @file device.h
 * @brief Main interface for interacting with Kinect hardware
 */

#pragma once

#include <string>

namespace kinect_xr {

enum class DeviceError { None = 0, DeviceNotFound, InitializationFailed, Unknown };

std::string errorToString(DeviceError error);

/**
 * @brief Main class for interacting with Kinect hardware
 */
class KinectDevice {
 public:
  KinectDevice();
  ~KinectDevice();
};

}  // namespace kinect_xr
