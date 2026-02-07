/**
 * @file device.h
 * @brief Main interface for interacting with Kinect hardware
 */

#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <functional>
#include <thread>

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
  MotorControlFailed,
  InvalidParameter,
  Unknown
};

/**
 * @brief LED states for Kinect LED
 */
enum class LEDState {
  Off = 0,
  Green = 1,
  Red = 2,
  Yellow = 3,
  BlinkGreen = 4,
  BlinkRedYellow = 6
};

/**
 * @brief Motor tilt status
 */
enum class TiltStatus {
  Stopped = 0x00,
  AtLimit = 0x01,
  Moving = 0x04
};

/**
 * @brief Motor status including angle and accelerometer
 */
struct MotorStatus {
  double tiltAngle;          ///< Current tilt angle in degrees (-27 to +27)
  TiltStatus status;         ///< Motor movement status
  double accelX;             ///< Accelerometer X (m/s²)
  double accelY;             ///< Accelerometer Y (m/s²)
  double accelZ;             ///< Accelerometer Z (m/s²)
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
 * @brief Callback types for frame data
 * @note For spike/prototyping use. Production code should use proper threading/buffering.
 */
using DepthCallback = std::function<void(const void* depth, uint32_t timestamp)>;
using VideoCallback = std::function<void(const void* rgb, uint32_t timestamp)>;

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

  /**
   * @brief Register callback for depth frames
   * @param callback Function to call when depth frame arrives
   * @note For spike/prototyping use. Called from libfreenect thread.
   */
  void setDepthCallback(DepthCallback callback);

  /**
   * @brief Register callback for RGB frames
   * @param callback Function to call when RGB frame arrives
   * @note For spike/prototyping use. Called from libfreenect thread.
   */
  void setVideoCallback(VideoCallback callback);

  /**
   * @brief Set motor tilt angle
   * @param degrees Target angle in degrees (-27 to +27, clamped to range)
   * @return DeviceError Error code, None if successful
   * @note Blocking call. Motor may still be moving when function returns.
   */
  DeviceError setTiltAngle(double degrees);

  /**
   * @brief Get current motor tilt angle
   * @param outAngle Output parameter for angle in degrees
   * @return DeviceError Error code, None if successful
   */
  DeviceError getTiltAngle(double& outAngle);

  /**
   * @brief Set Kinect LED state
   * @param state LED state to set
   * @return DeviceError Error code, None if successful
   */
  DeviceError setLED(LEDState state);

  /**
   * @brief Get complete motor status including angle and accelerometer
   * @param outStatus Output parameter for motor status
   * @return DeviceError Error code, None if successful
   */
  DeviceError getMotorStatus(MotorStatus& outStatus);

 private:
  freenect_context* ctx_;
  freenect_device* dev_;
  bool initialized_;
  std::atomic<bool> streaming_;
  DeviceConfig config_;

  // USB event processing thread
  std::thread eventThread_;
  std::atomic<bool> eventThreadRunning_;
  void eventLoop();

  // User callbacks
  DepthCallback depth_callback_;
  VideoCallback video_callback_;

  // Static C callback functions for libfreenect
  static void depthCallback(freenect_device* dev, void* depth, uint32_t timestamp);
  static void videoCallback(freenect_device* dev, void* rgb, uint32_t timestamp);
};

}  // namespace kinect_xr
