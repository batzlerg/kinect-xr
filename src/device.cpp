/**
 * @file device.cpp
 * @brief Implementation of Kinect device management
 */

#include "kinect_xr/device.h"

#include <libfreenect/libfreenect.h>

#include <iostream>

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
    case DeviceError::Unknown:
    default:
      return "Unknown error";
  }
}

KinectDevice::KinectDevice()
    : ctx_(nullptr), dev_(nullptr), initialized_(false), streaming_(false) {}

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
  return DeviceError::None;
}

DeviceError KinectDevice::stopStreams() {
  if (!initialized_) {
    return DeviceError::NotInitialized;
  }

  if (!streaming_) {
    return DeviceError::NotStreaming;
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

  // For now, just acknowledge receipt of depth data
  // Future phases will process and forward this data
  (void)depth;      // Suppress unused parameter warning
  (void)timestamp;  // Suppress unused parameter warning
}

void KinectDevice::videoCallback(freenect_device* dev, void* rgb, uint32_t timestamp) {
  // Get the KinectDevice instance from user data
  KinectDevice* device = static_cast<KinectDevice*>(freenect_get_user(dev));
  if (!device) {
    return;
  }

  // For now, just acknowledge receipt of video data
  // Future phases will process and forward this data
  (void)rgb;        // Suppress unused parameter warning
  (void)timestamp;  // Suppress unused parameter warning
}

}  // namespace kinect_xr
