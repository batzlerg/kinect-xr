/**
 * @file device.cpp
 * @brief Implementation of Kinect device management
 */

#include "kinect_xr/device.h"

#include <libfreenect/libfreenect.h>

#include <iostream>
#include <unistd.h>
#include <fcntl.h>

namespace kinect_xr {

std::string errorToString(DeviceError error) {
  switch (error) {
    case DeviceError::None:
      return "No error";
    case DeviceError::DeviceNotFound:
      return "Device not found";
    case DeviceError::InitializationFailed:
      return "Initialization failed";
    case DeviceError::NotInitialized:
      return "Device not initialized";
    case DeviceError::AlreadyStreaming:
      return "Streams already active";
    case DeviceError::NotStreaming:
      return "Streams not active";
    case DeviceError::MotorControlFailed:
      return "Motor control failed";
    case DeviceError::InvalidParameter:
      return "Invalid parameter";
    case DeviceError::Unknown:
    default:
      return "Unknown error";
  }
}

KinectDevice::KinectDevice()
    : ctx_(nullptr), dev_(nullptr), initialized_(false), streaming_(false), eventThreadRunning_(false) {}

KinectDevice::~KinectDevice() {
  if (initialized_) {
    // stop streams if active
    if (streaming_) {
      stopStreams();
    }

    // close device
    if (dev_) {
      freenect_close_device(dev_);
      dev_ = nullptr;
    }

    // shut down context
    if (ctx_) {
      freenect_shutdown(ctx_);
      ctx_ = nullptr;
    }

    initialized_ = false;
  }
}

DeviceError KinectDevice::initialize(const DeviceConfig& config) {
  if (initialized_) {
    return DeviceError::None;
  }

  // store configuration
  config_ = config;

  if (getDeviceCount() == 0) {
    return DeviceError::DeviceNotFound;
  }

  // initialize freenext context
  if (freenect_init(&ctx_, nullptr) < 0) {
    std::cerr << "Failed to initialize freenect context" << std::endl;
    return DeviceError::InitializationFailed;
  }

  // select subdevices
  freenect_device_flags subdevs =
      static_cast<freenect_device_flags>(0);  // initialize as 0 to represent no subdevice
  if (config.enableMotor) {
    subdevs = static_cast<freenect_device_flags>(subdevs | FREENECT_DEVICE_MOTOR);
  }
  if (config.enableRGB || config.enableDepth) {
    subdevs = static_cast<freenect_device_flags>(subdevs | FREENECT_DEVICE_CAMERA);
  }

  freenect_select_subdevices(ctx_, subdevs);

  // check if device is available
  int deviceCount = freenect_num_devices(ctx_);
  if (deviceCount <= 0) {
    freenect_shutdown(ctx_);
    ctx_ = nullptr;
    return DeviceError::DeviceNotFound;
  }

  // open device
  if (freenect_open_device(ctx_, &dev_, config.deviceId) < 0) {
    std::cerr << "Failed to open Kinect device " << config.deviceId << std::endl;
    freenect_shutdown(ctx_);
    ctx_ = nullptr;
    return DeviceError::InitializationFailed;
  }

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

bool KinectDevice::isStreaming() const { return streaming_; }

DeviceError KinectDevice::startStreams() {
  if (!initialized_) {
    return DeviceError::NotInitialized;
  }

  if (streaming_) {
    return DeviceError::AlreadyStreaming;
  }

  // M3: Register callbacks and start streams
  freenect_set_depth_callback(dev_, depthCallback);
  freenect_set_video_callback(dev_, videoCallback);

  // Set user data to allow callbacks to access this instance
  freenect_set_user(dev_, this);

  // Configure stream modes (MUST be done before starting streams)
  if (config_.enableDepth) {
    // Use FREENECT_DEPTH_MM to get actual millimeter values
    // (FREENECT_DEPTH_11BIT returns raw disparity which has inverse relationship)
    freenect_frame_mode depthMode = freenect_find_depth_mode(
        FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_MM);
    if (freenect_set_depth_mode(dev_, depthMode) < 0) {
      std::cerr << "Failed to set depth mode" << std::endl;
      return DeviceError::InitializationFailed;
    }
  }

  if (config_.enableRGB) {
    freenect_frame_mode videoMode = freenect_find_video_mode(
        FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB);
    if (freenect_set_video_mode(dev_, videoMode) < 0) {
      std::cerr << "Failed to set video mode" << std::endl;
      return DeviceError::InitializationFailed;
    }
  }

  // Start depth and video streams
  if (config_.enableDepth) {
    if (freenect_start_depth(dev_) < 0) {
      std::cerr << "Failed to start depth stream" << std::endl;
      return DeviceError::InitializationFailed;
    }
  }

  if (config_.enableRGB) {
    if (freenect_start_video(dev_) < 0) {
      std::cerr << "Failed to start video stream" << std::endl;
      // Clean up depth if it was started
      if (config_.enableDepth) {
        freenect_stop_depth(dev_);
      }
      return DeviceError::InitializationFailed;
    }
  }

  streaming_ = true;

  // Start USB event processing thread
  eventThreadRunning_ = true;
  eventThread_ = std::thread(&KinectDevice::eventLoop, this);

  return DeviceError::None;
}

void KinectDevice::eventLoop() {
  std::cout << "USB event loop started" << std::endl;

  // Suppress libfreenect USB error spam by redirecting stderr
  // (libfreenect logs "Invalid magic" errors that are non-fatal)
  int stderr_save = dup(STDERR_FILENO);
  int devnull = open("/dev/null", O_WRONLY);
  dup2(devnull, STDERR_FILENO);
  close(devnull);

  // Rate-limited error logging
  auto lastErrorLog = std::chrono::steady_clock::now();
  int errorsSinceLastLog = 0;

  while (eventThreadRunning_) {
    if (freenect_process_events(ctx_) < 0) {
      errorsSinceLastLog++;

      // Restore stderr temporarily for rate-limited logging
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastErrorLog).count();

      if (elapsed >= 10) {
        dup2(stderr_save, STDERR_FILENO);
        if (errorsSinceLastLog > 0) {
          std::cerr << "Warning: " << errorsSinceLastLog
                    << " USB errors in last " << elapsed << "s (non-fatal)" << std::endl;
        }
        errorsSinceLastLog = 0;
        lastErrorLog = now;

        // Suppress again
        int dn = open("/dev/null", O_WRONLY);
        dup2(dn, STDERR_FILENO);
        close(dn);
      }
    }
  }

  // Restore stderr
  dup2(stderr_save, STDERR_FILENO);
  close(stderr_save);

  std::cout << "USB event loop stopped" << std::endl;
}

DeviceError KinectDevice::stopStreams() {
  if (!initialized_) {
    return DeviceError::NotInitialized;
  }

  if (!streaming_) {
    return DeviceError::NotStreaming;
  }

  // Stop USB event processing thread first
  eventThreadRunning_ = false;
  if (eventThread_.joinable()) {
    eventThread_.join();
  }

  // Stop depth and video streams
  if (config_.enableDepth) {
    freenect_stop_depth(dev_);
  }

  if (config_.enableRGB) {
    freenect_stop_video(dev_);
  }

  streaming_ = false;
  return DeviceError::None;
}

// Static callback functions for libfreenect
void KinectDevice::depthCallback(freenect_device* dev, void* depth, uint32_t timestamp) {
  // Get the KinectDevice instance from user data
  KinectDevice* device = static_cast<KinectDevice*>(freenect_get_user(dev));
  if (!device) {
    return;
  }

  // Forward to user callback if registered
  if (device->depth_callback_) {
    device->depth_callback_(depth, timestamp);
  }
}

void KinectDevice::videoCallback(freenect_device* dev, void* rgb, uint32_t timestamp) {
  // Get the KinectDevice instance from user data
  KinectDevice* device = static_cast<KinectDevice*>(freenect_get_user(dev));
  if (!device) {
    return;
  }

  // Forward to user callback if registered
  if (device->video_callback_) {
    device->video_callback_(rgb, timestamp);
  }
}

void KinectDevice::setDepthCallback(DepthCallback callback) {
  depth_callback_ = callback;
}

void KinectDevice::setVideoCallback(VideoCallback callback) {
  video_callback_ = callback;
}

DeviceError KinectDevice::setTiltAngle(double degrees) {
  if (!initialized_) {
    return DeviceError::NotInitialized;
  }

  // Clamp to hardware limits (-27 to +27 degrees)
  double clamped = degrees;
  if (clamped < -27.0) clamped = -27.0;
  if (clamped > 27.0) clamped = 27.0;

  // Set tilt using libfreenect
  if (freenect_set_tilt_degs(dev_, clamped) < 0) {
    return DeviceError::MotorControlFailed;
  }

  return DeviceError::None;
}

DeviceError KinectDevice::getTiltAngle(double& outAngle) {
  if (!initialized_) {
    return DeviceError::NotInitialized;
  }

  // Update tilt state
  if (freenect_update_tilt_state(dev_) < 0) {
    return DeviceError::MotorControlFailed;
  }

  // Get tilt state
  freenect_raw_tilt_state* state = freenect_get_tilt_state(dev_);
  if (!state) {
    return DeviceError::MotorControlFailed;
  }

  outAngle = freenect_get_tilt_degs(state);
  return DeviceError::None;
}

DeviceError KinectDevice::setLED(LEDState state) {
  if (!initialized_) {
    return DeviceError::NotInitialized;
  }

  // Map LEDState enum to libfreenect constants
  freenect_led_options led_option;
  switch (state) {
    case LEDState::Off:
      led_option = LED_OFF;
      break;
    case LEDState::Green:
      led_option = LED_GREEN;
      break;
    case LEDState::Red:
      led_option = LED_RED;
      break;
    case LEDState::Yellow:
      led_option = LED_YELLOW;
      break;
    case LEDState::BlinkGreen:
      led_option = LED_BLINK_GREEN;
      break;
    case LEDState::BlinkRedYellow:
      led_option = LED_BLINK_RED_YELLOW;
      break;
    default:
      return DeviceError::InvalidParameter;
  }

  if (freenect_set_led(dev_, led_option) < 0) {
    return DeviceError::MotorControlFailed;
  }

  return DeviceError::None;
}

DeviceError KinectDevice::getMotorStatus(MotorStatus& outStatus) {
  if (!initialized_) {
    return DeviceError::NotInitialized;
  }

  // Update tilt state
  if (freenect_update_tilt_state(dev_) < 0) {
    return DeviceError::MotorControlFailed;
  }

  // Get tilt state
  freenect_raw_tilt_state* state = freenect_get_tilt_state(dev_);
  if (!state) {
    return DeviceError::MotorControlFailed;
  }

  // Get tilt angle
  outStatus.tiltAngle = freenect_get_tilt_degs(state);

  // Map tilt status
  freenect_tilt_status_code status_code = freenect_get_tilt_status(state);
  switch (status_code) {
    case TILT_STATUS_STOPPED:
      outStatus.status = TiltStatus::Stopped;
      break;
    case TILT_STATUS_LIMIT:
      outStatus.status = TiltStatus::AtLimit;
      break;
    case TILT_STATUS_MOVING:
      outStatus.status = TiltStatus::Moving;
      break;
    default:
      outStatus.status = TiltStatus::Stopped;
  }

  // Get accelerometer data
  freenect_get_mks_accel(state, &outStatus.accelX, &outStatus.accelY, &outStatus.accelZ);

  return DeviceError::None;
}

}  // namespace kinect_xr
