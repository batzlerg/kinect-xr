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
    case DeviceError::Unknown:
    default:
      return "Unknown error";
  }
}

KinectDevice::KinectDevice() : ctx_(nullptr), dev_(nullptr), initialized_(false) {}

KinectDevice::~KinectDevice() {
  if (initialized_) {
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

}  // namespace kinect_xr
