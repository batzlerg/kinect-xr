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
  /**
   * @brief Get count of available devices
   * @return int Number of connected Kinect devices
   */
  static int getDeviceCount();
};

}  // namespace kinect_xr
