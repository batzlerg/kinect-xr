/**
 * @file device.h
 * @brief Main interface for interacting with Kinect hardware
 */

#pragma once

#include <memory>
#include <string>

struct _freenect_context;
typedef struct _freenect_context freenect_context;
struct _freenect_device;
typedef struct _freenect_device freenect_device;

namespace kinect_xr {

enum class DeviceError {
  None = 0,
  DeviceNotFound,
  InitializationFailed,
  NotInitialized,
  AlreadyStreaming,
  NotStreaming,
  Unknown
};

std::string errorToString(DeviceError error);

/**
 * @brief Configuration for the Kinect device
 */
struct DeviceConfig {
  bool enableRGB = true;    ///< Enable RGB camera
  bool enableDepth = true;  ///< Enable depth sensor
  bool enableMotor = true;  ///< Enable motor control
  int deviceId = 0;         ///< Device ID to open (todo: buy another Kinect and test for real)
};

/**
 * @brief Main class for interacting with Kinect hardware
 */
class KinectDevice {
 public:
  KinectDevice();
  ~KinectDevice();

  // avoid double-freeing by making instance 1:1 with physical device
  KinectDevice(const KinectDevice&) = delete;
  KinectDevice& operator=(const KinectDevice&) = delete;

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
   * @brief Start depth and RGB streams
   * @return DeviceError Error code, None if successful
   *
   * Starts continuous capture from depth and RGB sensors. Frame data will be delivered
   * via libfreenect callbacks. Device must be initialized before calling.
   * Returns AlreadyStreaming if streams are already active.
   */
  DeviceError startStreams();

  /**
   * @brief Stop depth and RGB streams
   * @return DeviceError Error code, None if successful
   *
   * Stops continuous capture and halts callback execution.
   * Returns NotStreaming if streams are not active.
   */
  DeviceError stopStreams();

  /**
   * @brief Check if streams are currently active
   * @return bool True if streams are running
   */
  bool isStreaming() const;

  /**
   * @brief Get count of available devices
   * @return int Number of connected Kinect devices
   */
  static int getDeviceCount();

 private:
  freenect_context* ctx_;
  freenect_device* dev_;
  bool initialized_;
  bool streaming_;
  DeviceConfig config_;

  // Static C callback functions for libfreenect
  static void depthCallback(freenect_device* dev, void* depth, uint32_t timestamp);
  static void videoCallback(freenect_device* dev, void* rgb, uint32_t timestamp);
};

}  // namespace kinect_xr
