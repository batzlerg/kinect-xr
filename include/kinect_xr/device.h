/**
 * @file device.h
 * @brief Main interface for interacting with Kinect hardware
 */

#pragma once

#include <memory>
#include <string>

namespace kinect_xr {

enum class DeviceError { None = 0, DeviceNotFound, InitializationFailed, Unknown };

std::string errorToString(DeviceError error);

/**
 * @brief Configuration for the Kinect device
 */
struct DeviceConfig {
  bool enableRGB = true;    ///< Enable RGB camera
  bool enableDepth = true;  ///< Enable depth sensor
  bool enableMotor = true;  ///< Enable motor control
};

/**
 * @brief Main class for interacting with Kinect hardware
 */
class KinectDevice {
 public:
  KinectDevice();
  ~KinectDevice();
  /**
   * @brief Initialize the Kinect device
   * @param config Configuration settings
   * @return DeviceError Error code, None if successful
   */
  DeviceError initialize(const DeviceConfig& config);

  /**
   * @brief Check if device is initialized
   * @return bool True if initialized
   */
  bool isInitialized() const;

  /**
   * @brief Get count of available devices
   * @return int Number of connected Kinect devices
   */
  static int getDeviceCount();

 private:
  bool initialized_ = false;
  DeviceConfig config_;
};

}  // namespace kinect_xr
