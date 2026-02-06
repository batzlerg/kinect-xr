<img src="https://r2cdn.perplexity.ai/pplx-full-logo-primary-dark%402x.png" class="logo" width="120"/>

# Comprehensive Implementation Guide for Kinect XR - Phase 1

This detailed guide provides step-by-step instructions for implementing Phase 1 of the Kinect XR project on macOS, covering development environment setup and Kinect device integration.

## Prerequisites Installation

Begin by installing required dependencies on your macOS system:

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required dependencies
brew install cmake
brew install libfreenect
brew install googletest

# Optional visualization tools
brew install opencv
```


## Project Structure Setup

Create the following directory structure to organize your project effectively:

```
kinect-xr/
├── CMakeLists.txt
├── cmake/
│   └── FindLibfreenect.cmake
├── include/
│   └── kinect_xr/
│       ├── device.h
│       ├── streams.h
│       └── motor.h
├── src/
│   ├── device.cpp
│   ├── streams.cpp
│   └── motor.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── device_tests.cpp
│   └── stream_tests.cpp
└── examples/
    ├── CMakeLists.txt
    ├── device_info.cpp
    └── stream_viewer.cpp
```


## CMake Configuration

### Main CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.18)
project(KinectXR VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add cmake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Options
option(KINECT_XR_BUILD_TESTS "Build tests" ON)
option(KINECT_XR_BUILD_EXAMPLES "Build examples" ON)

# Find dependencies
find_package(LibFreenect REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBFREENECT_INCLUDE_DIRS}
)

# Define library
add_library(kinect_xr
    src/device.cpp
    src/streams.cpp
    src/motor.cpp
)

# Link dependencies
target_link_libraries(kinect_xr
    ${LIBFREENECT_LIBRARIES}
)

# Installation targets
install(TARGETS kinect_xr
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
)

# Build tests if enabled
if(KINECT_XR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Build examples if enabled
if(KINECT_XR_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Output configuration info
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
```


### Custom Find Module for libfreenect

Create `cmake/FindLibfreenect.cmake`:

```cmake
# FindLibFreenect.cmake
# Locates libfreenect library and include directories

find_path(LIBFREENECT_INCLUDE_DIR
    NAMES libfreenect.h
    PATHS
        /usr/include/libfreenect
        /usr/local/include/libfreenect
        /opt/local/include/libfreenect
    DOC "libfreenect include directory"
)

find_library(LIBFREENECT_LIBRARY
    NAMES freenect
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
    DOC "libfreenect library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibFreenect
    DEFAULT_MSG
    LIBFREENECT_LIBRARY LIBFREENECT_INCLUDE_DIR
)

if(LIBFREENECT_FOUND)
    set(LIBFREENECT_LIBRARIES ${LIBFREENECT_LIBRARY})
    set(LIBFREENECT_INCLUDE_DIRS ${LIBFREENECT_INCLUDE_DIR})
endif()

mark_as_advanced(LIBFREENECT_LIBRARY LIBFREENECT_INCLUDE_DIR)
```


### Test CMakeLists.txt

Create `tests/CMakeLists.txt`:

```cmake
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(device_tests device_tests.cpp)
target_link_libraries(device_tests
    kinect_xr
    ${GTEST_BOTH_LIBRARIES}
    pthread
)

add_executable(stream_tests stream_tests.cpp)
target_link_libraries(stream_tests
    kinect_xr
    ${GTEST_BOTH_LIBRARIES}
    pthread
)

add_test(NAME DeviceTests COMMAND device_tests)
add_test(NAME StreamTests COMMAND stream_tests)
```


### Examples CMakeLists.txt

Create `examples/CMakeLists.txt`:

```cmake
add_executable(device_info device_info.cpp)
target_link_libraries(device_info kinect_xr)

add_executable(stream_viewer stream_viewer.cpp)
target_link_libraries(stream_viewer kinect_xr)

# Optional: Add OpenCV for visualization if available
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    add_definitions(-DUSE_OPENCV)
    include_directories(${OpenCV_INCLUDE_DIRS})
    target_link_libraries(stream_viewer ${OpenCV_LIBS})
endif()
```


## Core Implementation: Device Management

### Device Header (include/kinect_xr/device.h)

```cpp
/**
 * @file device.h
 * @brief Kinect device management interface
 */

#pragma once

#include &lt;vector&gt;
#include &lt;memory&gt;
#include &lt;string&gt;
#include &lt;functional&gt;
#include &lt;mutex&gt;
#include &lt;thread&gt;
#include &lt;atomic&gt;

// Forward declarations of libfreenect types to avoid exposing in public API
struct _freenect_context;
typedef struct _freenect_context freenect_context;
struct _freenect_device;
typedef struct _freenect_device freenect_device;

namespace kinect_xr {

/**
 * @brief Error codes for Kinect operations
 */
enum class DeviceError {
    None = 0,
    DeviceNotFound,
    InitializationFailed,
    StreamStartFailed,
    StreamAlreadyRunning,
    MotorControlFailed,
    InvalidParameter,
    DeviceDisconnected,
    Unknown
};

/**
 * @brief String representation of error codes
 */
std::string errorToString(DeviceError error);

/**
 * @brief Kinect device information
 */
struct DeviceInfo {
    int id;                    ///&lt; Device ID (index)
    std::string serialNumber;  ///&lt; Serial number (if available)
    int cameraCount;           ///&lt; Number of cameras
    bool hasMotor;             ///&lt; Has motor control capability
    bool hasAccelerometer;     ///&lt; Has accelerometer capability
    bool hasLED;               ///&lt; Has LED capability
};

/**
 * @brief Configuration for the Kinect device
 */
struct DeviceConfig {
    bool enableRGB = true;       ///&lt; Enable RGB camera
    bool enableDepth = true;     ///&lt; Enable depth camera
    bool enableMotor = false;    ///&lt; Enable motor control
    bool enableLED = false;      ///&lt; Enable LED control
    int rgbResolution = 1;       ///&lt; RGB resolution (0=QVGA, 1=VGA)
    int depthResolution = 1;     ///&lt; Depth resolution (0=QVGA, 1=VGA)
    int rgbFormat = 0;           ///&lt; RGB format (0=RGB, 1=YUV)
    int depthFormat = 0;         ///&lt; Depth format (0=11BIT, 1=10BIT, 2=PACKED)
};

/**
 * @brief Main class for managing a Kinect device
 */
class KinectDevice {
public:
    /**
     * @brief Constructor
     * @param deviceId The ID of the device to open (default: 0 for first device)
     */
    explicit KinectDevice(int deviceId = 0);
    
    /**
     * @brief Destructor - ensures device is properly closed
     */
    ~KinectDevice();
    
    // Delete copy constructor and assignment operator
    KinectDevice(const KinectDevice&amp;) = delete;
    KinectDevice&amp; operator=(const KinectDevice&amp;) = delete;
    
    /**
     * @brief Initialize the device with the given configuration
     * @param config Configuration options
     * @return DeviceError Error code (None for success)
     */
    DeviceError initialize(const DeviceConfig&amp; config = DeviceConfig());
    
    /**
     * @brief Start streaming data from the device
     * @return DeviceError Error code (None for success)
     */
    DeviceError startStreams();
    
    /**
     * @brief Stop streaming data from the device
     * @return DeviceError Error code (None for success)
     */
    DeviceError stopStreams();
    
    /**
     * @brief Check if the device is initialized
     * @return bool True if initialized
     */
    bool isInitialized() const;
    
    /**
     * @brief Check if streams are running
     * @return bool True if streaming
     */
    bool isStreaming() const;
    
    /**
     * @brief Get information about the device
     * @return DeviceInfo Structure with device information
     */
    DeviceInfo getDeviceInfo() const;
    
    /**
     * @brief Set the tilt angle of the Kinect motor
     * @param angleInDegrees Angle in degrees (-31 to 31)
     * @return DeviceError Error code (None for success)
     */
    DeviceError setTiltAngle(double angleInDegrees);
    
    /**
     * @brief Get the current tilt angle
     * @param angleInDegrees Output parameter for the angle
     * @return DeviceError Error code (None for success)
     */
    DeviceError getCurrentTiltAngle(double&amp; angleInDegrees) const;
    
    /**
     * @brief Reset the tilt angle to 0
     * @return DeviceError Error code (None for success)
     */
    DeviceError resetTiltAngle();
    
    /**
     * @brief Get a reference to the depth data buffer
     * @return const std::vector&lt;uint16_t&gt;&amp; Depth data buffer
     */
    const std::vector&lt;uint16_t&gt;&amp; getDepthData() const;
    
    /**
     * @brief Get a reference to the RGB data buffer
     * @return const std::vector&lt;uint8_t&gt;&amp; RGB data buffer
     */
    const std::vector&lt;uint8_t&gt;&amp; getRgbData() const;
    
    /**
     * @brief Get count of available devices
     * @return int Number of connected Kinect devices
     */
    static int getDeviceCount();
    
    /**
     * @brief Register callback for depth frame updates
     * @param callback Function to call when depth frame is updated
     */
    void setDepthCallback(std::function&lt;void(const std::vector&lt;uint16_t&gt;&amp;)&gt; callback);
    
    /**
     * @brief Register callback for RGB frame updates
     * @param callback Function to call when RGB frame is updated
     */
    void setRgbCallback(std::function&lt;void(const std::vector&lt;uint8_t&gt;&amp;)&gt; callback);

private:
    // Static callbacks for libfreenect
    static void depthCallback(freenect_device* dev, void* depth, uint32_t timestamp);
    static void rgbCallback(freenect_device* dev, void* rgb, uint32_t timestamp);
    
    // Process callbacks in non-static context
    void handleDepthData(void* depth, uint32_t timestamp);
    void handleRgbData(void* rgb, uint32_t timestamp);
    
    // Thread for processing libfreenect events
    void processEvents();
    
    // Member variables
    int deviceId_;
    freenect_context* ctx_;
    freenect_device* dev_;
    
    std::vector&lt;uint16_t&gt; depthBuffer_;
    std::vector&lt;uint8_t&gt; rgbBuffer_;
    
    std::mutex depthMutex_;
    std::mutex rgbMutex_;
    
    std::atomic&lt;bool&gt; initialized_;
    std::atomic&lt;bool&gt; streaming_;
    std::atomic&lt;bool&gt; shouldRun_;
    
    std::unique_ptr&lt;std::thread&gt; eventThread_;
    
    DeviceConfig config_;
    DeviceInfo info_;
    
    std::function&lt;void(const std::vector&lt;uint16_t&gt;&amp;)&gt; depthCallback_;
    std::function&lt;void(const std::vector&lt;uint8_t&gt;&amp;)&gt; rgbCallback_;
};

/**
 * @brief Get list of all connected Kinect devices
 * @return std::vector&lt;DeviceInfo&gt; List of device information
 */
std::vector&lt;DeviceInfo&gt; getConnectedDevices();

} // namespace kinect_xr
```


### Device Implementation (src/device.cpp)

```cpp
/**
 * @file device.cpp
 * @brief Implementation of Kinect device management
 */

#include "kinect_xr/device.h"
#include &lt;libfreenect/libfreenect.h&gt;
#include &lt;iostream&gt;
#include &lt;chrono&gt;
#include &lt;algorithm&gt;
#include &lt;cstring&gt;

namespace kinect_xr {

// Error string mapping
std::string errorToString(DeviceError error) {
    switch (error) {
        case DeviceError::None: return "No error";
        case DeviceError::DeviceNotFound: return "Device not found";
        case DeviceError::InitializationFailed: return "Initialization failed";
        case DeviceError::StreamStartFailed: return "Stream start failed";
        case DeviceError::StreamAlreadyRunning: return "Stream already running";
        case DeviceError::MotorControlFailed: return "Motor control failed";
        case DeviceError::InvalidParameter: return "Invalid parameter";
        case DeviceError::DeviceDisconnected: return "Device disconnected";
        case DeviceError::Unknown:
        default: return "Unknown error";
    }
}

// Constants for buffer sizes
constexpr int DEPTH_WIDTH_QVGA = 320;
constexpr int DEPTH_HEIGHT_QVGA = 240;
constexpr int DEPTH_WIDTH_VGA = 640;
constexpr int DEPTH_HEIGHT_VGA = 480;
constexpr int RGB_WIDTH_QVGA = 320;
constexpr int RGB_HEIGHT_QVGA = 240;
constexpr int RGB_WIDTH_VGA = 640;
constexpr int RGB_HEIGHT_VGA = 480;
constexpr int RGB_CHANNELS = 3; // RGB

KinectDevice::KinectDevice(int deviceId) 
    : deviceId_(deviceId),
      ctx_(nullptr),
      dev_(nullptr),
      initialized_(false),
      streaming_(false),
      shouldRun_(false) {
}

KinectDevice::~KinectDevice() {
    // Ensure streams are stopped and device is closed
    if (streaming_) {
        stopStreams();
    }
    
    if (initialized_) {
        // Close device
        if (dev_) {
            freenect_close_device(dev_);
            dev_ = nullptr;
        }
        
        // Shutdown context
        if (ctx_) {
            freenect_shutdown(ctx_);
            ctx_ = nullptr;
        }
        
        initialized_ = false;
    }
}

DeviceError KinectDevice::initialize(const DeviceConfig&amp; config) {
    if (initialized_) {
        // Already initialized
        return DeviceError::None;
    }
    
    // Store configuration
    config_ = config;
    
    // Initialize context
    if (freenect_init(&amp;ctx_, nullptr) &lt; 0) {
        return DeviceError::InitializationFailed;
    }
    
    // Set log level
    freenect_set_log_level(ctx_, FREENECT_LOG_INFO);
    
    // Select subdevices to enable
    freenect_device_flags subdevs = FREENECT_DEVICE_NONE;
    if (config.enableMotor || config.enableLED) {
        subdevs = static_cast&lt;freenect_device_flags&gt;(subdevs | FREENECT_DEVICE_MOTOR);
    }
    if (config.enableRGB) {
        subdevs = static_cast&lt;freenect_device_flags&gt;(subdevs | FREENECT_DEVICE_CAMERA);
    }
    if (config.enableDepth) {
        subdevs = static_cast&lt;freenect_device_flags&gt;(subdevs | FREENECT_DEVICE_CAMERA);
    }
    
    freenect_select_subdevices(ctx_, subdevs);
    
    // Check if device is available
    int deviceCount = freenect_num_devices(ctx_);
    if (deviceCount &lt;= deviceId_) {
        freenect_shutdown(ctx_);
        ctx_ = nullptr;
        return DeviceError::DeviceNotFound;
    }
    
    // Open device
    if (freenect_open_device(ctx_, &amp;dev_, deviceId_) &lt; 0) {
        freenect_shutdown(ctx_);
        ctx_ = nullptr;
        return DeviceError::InitializationFailed;
    }
    
    // Store device info
    info_.id = deviceId_;
    info_.cameraCount = 2; // RGB and depth
    info_.hasMotor = (subdevs &amp; FREENECT_DEVICE_MOTOR) != 0;
    info_.hasAccelerometer = true; // Kinect always has accelerometer
    info_.hasLED = (subdevs &amp; FREENECT_DEVICE_MOTOR) != 0; // LED is part of motor subdevice
    
    // Set user data for callbacks
    freenect_set_user(dev_, this);
    
    // Initialize buffers based on selected resolution
    if (config.enableDepth) {
        int depthWidth = (config.depthResolution == 0) ? DEPTH_WIDTH_QVGA : DEPTH_WIDTH_VGA;
        int depthHeight = (config.depthResolution == 0) ? DEPTH_HEIGHT_QVGA : DEPTH_HEIGHT_VGA;
        depthBuffer_.resize(depthWidth * depthHeight);
    }
    
    if (config.enableRGB) {
        int rgbWidth = (config.rgbResolution == 0) ? RGB_WIDTH_QVGA : RGB_WIDTH_VGA;
        int rgbHeight = (config.rgbResolution == 0) ? RGB_HEIGHT_QVGA : RGB_HEIGHT_VGA;
        rgbBuffer_.resize(rgbWidth * rgbHeight * RGB_CHANNELS);
    }
    
    // Setup callbacks
    if (config.enableDepth) {
        freenect_set_depth_callback(dev_, depthCallback);
    }
    
    if (config.enableRGB) {
        freenect_set_video_callback(dev_, rgbCallback);
    }
    
    initialized_ = true;
    return DeviceError::None;
}

DeviceError KinectDevice::startStreams() {
    if (!initialized_) {
        return DeviceError::InitializationFailed;
    }
    
    if (streaming_) {
        return DeviceError::StreamAlreadyRunning;
    }
    
    // Configure and start depth stream
    if (config_.enableDepth) {
        // Configure depth format
        freenect_depth_format depthFormat;
        switch (config_.depthFormat) {
            case 0: depthFormat = FREENECT_DEPTH_11BIT; break;
            case 1: depthFormat = FREENECT_DEPTH_10BIT; break;
            case 2: depthFormat = FREENECT_DEPTH_REGISTERED; break;
            default: depthFormat = FREENECT_DEPTH_11BIT;
        }
        
        // Configure resolution
        if (config_.depthResolution == 0) {
            freenect_set_depth_mode(dev_, freenect_find_depth_mode(
                FREENECT_RESOLUTION_MEDIUM, depthFormat));
        } else {
            freenect_set_depth_mode(dev_, freenect_find_depth_mode(
                FREENECT_RESOLUTION_HIGH, depthFormat));
        }
        
        // Start depth stream
        if (freenect_start_depth(dev_) &lt; 0) {
            return DeviceError::StreamStartFailed;
        }
    }
    
    // Configure and start video stream
    if (config_.enableRGB) {
        // Configure video format
        freenect_video_format videoFormat;
        switch (config_.rgbFormat) {
            case 0: videoFormat = FREENECT_VIDEO_RGB; break;
            case 1: videoFormat = FREENECT_VIDEO_YUV_RGB; break;
            default: videoFormat = FREENECT_VIDEO_RGB;
        }
        
        // Configure resolution
        if (config_.rgbResolution == 0) {
            freenect_set_video_mode(dev_, freenect_find_video_mode(
                FREENECT_RESOLUTION_MEDIUM, videoFormat));
        } else {
            freenect_set_video_mode(dev_, freenect_find_video_mode(
                FREENECT_RESOLUTION_HIGH, videoFormat));
        }
        
        // Start video stream
        if (freenect_start_video(dev_) &lt; 0) {
            if (config_.enableDepth) {
                freenect_stop_depth(dev_);
            }
            return DeviceError::StreamStartFailed;
        }
    }
    
    // Start event processing thread
    shouldRun_ = true;
    eventThread_ = std::make_unique&lt;std::thread&gt;(&amp;KinectDevice::processEvents, this);
    
    streaming_ = true;
    return DeviceError::None;
}

DeviceError KinectDevice::stopStreams() {
    if (!streaming_) {
        return DeviceError::None;
    }
    
    // Stop streams
    if (config_.enableDepth) {
        freenect_stop_depth(dev_);
    }
    
    if (config_.enableRGB) {
        freenect_stop_video(dev_);
    }
    
    // Stop event thread
    shouldRun_ = false;
    if (eventThread_ &amp;&amp; eventThread_-&gt;joinable()) {
        eventThread_-&gt;join();
    }
    
    streaming_ = false;
    return DeviceError::None;
}

bool KinectDevice::isInitialized() const {
    return initialized_;
}

bool KinectDevice::isStreaming() const {
    return streaming_;
}

DeviceInfo KinectDevice::getDeviceInfo() const {
    return info_;
}

DeviceError KinectDevice::setTiltAngle(double angleInDegrees) {
    if (!initialized_ || !dev_) {
        return DeviceError::InitializationFailed;
    }
    
    if (!info_.hasMotor) {
        return DeviceError::MotorControlFailed;
    }
    
    // Clamp angle to valid range
    angleInDegrees = std::max(-31.0, std::min(31.0, angleInDegrees));
    
    // Set motor angle
    if (freenect_set_tilt_degs(dev_, static_cast&lt;double&gt;(angleInDegrees)) != 0) {
        return DeviceError::MotorControlFailed;
    }
    
    return DeviceError::None;
}

DeviceError KinectDevice::getCurrentTiltAngle(double&amp; angleInDegrees) const {
    if (!initialized_ || !dev_) {
        return DeviceError::InitializationFailed;
    }
    
    if (!info_.hasMotor) {
        return DeviceError::MotorControlFailed;
    }
    
    // Query motor state
    freenect_raw_tilt_state* state = nullptr;
    state = freenect_get_tilt_state(dev_);
    if (!state) {
        return DeviceError::MotorControlFailed;
    }
    
    // Convert to degrees
    angleInDegrees = freenect_get_tilt_degs(state);
    
    return DeviceError::None;
}

DeviceError KinectDevice::resetTiltAngle() {
    return setTiltAngle(0.0);
}

const std::vector&lt;uint16_t&gt;&amp; KinectDevice::getDepthData() const {
    return depthBuffer_;
}

const std::vector&lt;uint8_t&gt;&amp; KinectDevice::getRgbData() const {
    return rgbBuffer_;
}

int KinectDevice::getDeviceCount() {
    freenect_context* ctx = nullptr;
    if (freenect_init(&amp;ctx, nullptr) &lt; 0) {
        return 0;
    }
    
    int count = freenect_num_devices(ctx);
    freenect_shutdown(ctx);
    
    return count;
}

void KinectDevice::setDepthCallback(std::function&lt;void(const std::vector&lt;uint16_t&gt;&amp;)&gt; callback) {
    depthCallback_ = callback;
}

void KinectDevice::setRgbCallback(std::function&lt;void(const std::vector&lt;uint8_t&gt;&amp;)&gt; callback) {
    rgbCallback_ = callback;
}

// Static callback forwarding to the instance
void KinectDevice::depthCallback(freenect_device* dev, void* depth, uint32_t timestamp) {
    KinectDevice* device = static_cast&lt;KinectDevice*&gt;(freenect_get_user(dev));
    if (device) {
        device-&gt;handleDepthData(depth, timestamp);
    }
}

// Static callback forwarding to the instance
void KinectDevice::rgbCallback(freenect_device* dev, void* rgb, uint32_t timestamp) {
    KinectDevice* device = static_cast&lt;KinectDevice*&gt;(freenect_get_user(dev));
    if (device) {
        device-&gt;handleRgbData(rgb, timestamp);
    }
}

void KinectDevice::handleDepthData(void* depth, uint32_t timestamp) {
    if (!depth || depthBuffer_.empty()) return;
    
    // Lock for thread-safe access to the buffer
    std::lock_guard&lt;std::mutex&gt; lock(depthMutex_);
    
    // Copy depth data to our buffer
    std::memcpy(depthBuffer_.data(), depth, depthBuffer_.size() * sizeof(uint16_t));
    
    // Call user callback if registered
    if (depthCallback_) {
        depthCallback_(depthBuffer_);
    }
}

void KinectDevice::handleRgbData(void* rgb, uint32_t timestamp) {
    if (!rgb || rgbBuffer_.empty()) return;
    
    // Lock for thread-safe access to the buffer
    std::lock_guard&lt;std::mutex&gt; lock(rgbMutex_);
    
    // Copy RGB data to our buffer
    std::memcpy(rgbBuffer_.data(), rgb, rgbBuffer_.size() * sizeof(uint8_t));
    
    // Call user callback if registered
    if (rgbCallback_) {
        rgbCallback_(rgbBuffer_);
    }
}

void KinectDevice::processEvents() {
    while (shouldRun_ &amp;&amp; initialized_) {
        if (freenect_process_events(ctx_) &lt; 0) {
            // Error processing events
            break;
        }
        
        // Add a small sleep to avoid consuming 100% CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

std::vector&lt;DeviceInfo&gt; getConnectedDevices() {
    std::vector&lt;DeviceInfo&gt; devices;
    
    freenect_context* ctx = nullptr;
    if (freenect_init(&amp;ctx, nullptr) &lt; 0) {
        return devices;
    }
    
    int count = freenect_num_devices(ctx);
    
    for (int i = 0; i &lt; count; i++) {
        freenect_device* dev = nullptr;
        if (freenect_open_device(ctx, &amp;dev, i) == 0) {
            DeviceInfo info;
            info.id = i;
            info.cameraCount = 2;
            info.hasMotor = true;
            info.hasAccelerometer = true;
            info.hasLED = true;
            
            // Get serial number if available
            char serial[^1024];
            if (freenect_get_device_serial(dev, serial, 1024) == 0) {
                info.serialNumber = serial;
            }
            
            freenect_close_device(dev);
            devices.push_back(info);
        }
    }
    
    freenect_shutdown(ctx);
    
    return devices;
}

} // namespace kinect_xr
```


## Streams Interface

### Streams Header (include/kinect_xr/streams.h)

```cpp
/**
 * @file streams.h
 * @brief Stream handling interfaces for Kinect data
 */

#pragma once

#include &lt;vector&gt;
#include &lt;cstdint&gt;
#include &lt;memory&gt;
#include &lt;functional&gt;
#include &lt;mutex&gt;

namespace kinect_xr {

/**
 * @brief Resolution options for streams
 */
enum class StreamResolution {
    QVGA,  ///&lt; 320x240
    VGA    ///&lt; 640x480
};

/**
 * @brief Types of streams available
 */
enum class StreamType {
    Depth,  ///&lt; Depth stream
    RGB     ///&lt; RGB stream
};

/**
 * @brief Base class for stream data
 */
class StreamData {
public:
    virtual ~StreamData() = default;
    
    /**
     * @brief Get the width of the stream
     * @return int Width in pixels
     */
    virtual int getWidth() const = 0;
    
    /**
     * @brief Get the height of the stream
     * @return int Height in pixels
     */
    virtual int getHeight() const = 0;
    
    /**
     * @brief Get the timestamp of the frame
     * @return uint32_t Timestamp in milliseconds
     */
    virtual uint32_t getTimestamp() const = 0;
};

/**
 * @brief Depth stream data
 */
class DepthStreamData : public StreamData {
public:
    /**
     * @brief Constructor
     * @param width Width in pixels
     * @param height Height in pixels
     * @param timestamp Frame timestamp
     * @param data Raw depth data
     */
    DepthStreamData(int width, int height, uint32_t timestamp, const std::vector&lt;uint16_t&gt;&amp; data);
    
    /**
     * @brief Get the width of the stream
     * @return int Width in pixels
     */
    int getWidth() const override;
    
    /**
     * @brief Get the height of the stream
     * @return int Height in pixels
     */
    int getHeight() const override;
    
    /**
     * @brief Get the timestamp of the frame
     * @return uint32_t Timestamp in milliseconds
     */
    uint32_t getTimestamp() const override;
    
    /**
     * @brief Get the raw depth data
     * @return const std::vector&lt;uint16_t&gt;&amp; Raw depth data
     */
    const std::vector&lt;uint16_t&gt;&amp; getData() const;
    
    /**
     * @brief Get depth value at specific pixel
     * @param x X coordinate
     * @param y Y coordinate
     * @return uint16_t Depth value (in millimeters)
     */
    uint16_t getDepthAt(int x, int y) const;
    
    /**
     * @brief Convert raw depth to meters
     * @param rawDepth Raw depth value
     * @return float Depth in meters
     */
    static float rawDepthToMeters(uint16_t rawDepth);

private:
    int width_;
    int height_;
    uint32_t timestamp_;
    std::vector&lt;uint16_t&gt; data_;
};

/**
 * @brief RGB stream data
 */
class RgbStreamData : public StreamData {
public:
    /**
     * @brief Constructor
     * @param width Width in pixels
     * @param height Height in pixels
     * @param timestamp Frame timestamp
     * @param data Raw RGB data
     */
    RgbStreamData(int width, int height, uint32_t timestamp, const std::vector&lt;uint8_t&gt;&amp; data);
    
    /**
     * @brief Get the width of the stream
     * @return int Width in pixels
     */
    int getWidth() const override;
    
    /**
     * @brief Get the height of the stream
     * @return int Height in pixels
     */
    int getHeight() const override;
    
    /**
     * @brief Get the timestamp of the frame
     * @return uint32_t Timestamp in milliseconds
     */
    uint32_t getTimestamp() const override;
    
    /**
     * @brief Get the raw RGB data
     * @return const std::vector&lt;uint8_t&gt;&amp; Raw RGB data
     */
    const std::vector&lt;uint8_t&gt;&amp; getData() const;
    
    /**
     * @brief Get RGB value at specific pixel
     * @param x X coordinate
     * @param y Y coordinate
     * @param r Output red component
     * @param g Output green component
     * @param b Output blue component
     */
    void getRgbAt(int x, int y, uint8_t&amp; r, uint8_t&amp; g, uint8_t&amp; b) const;

private:
    int width_;
    int height_;
    uint32_t timestamp_;
    std::vector&lt;uint8_t&gt; data_;
};

} // namespace kinect_xr
```


## Testing Components

### Device Tests (tests/device_tests.cpp)

```cpp
#include &lt;gtest/gtest.h&gt;
#include "kinect_xr/device.h"

using namespace kinect_xr;

TEST(KinectDeviceTest, GetDeviceCount) {
    int count = KinectDevice::getDeviceCount();
    std::cout &lt;&lt; "Found " &lt;&lt; count &lt;&lt; " Kinect devices" &lt;&lt; std::endl;
    // We don't fail if no devices are found, but we output the count
}

TEST(KinectDeviceTest, GetConnectedDevices) {
    auto devices = getConnectedDevices();
    for (const auto&amp; device : devices) {
        std::cout &lt;&lt; "Device ID: " &lt;&lt; device.id &lt;&lt; std::endl;
        std::cout &lt;&lt; "Serial: " &lt;&lt; device.serialNumber &lt;&lt; std::endl;
    }
    // We don't fail if no devices are found, but we output the info
}

TEST(KinectDeviceTest, Initialize) {
    // Only run this test if devices are available
    if (KinectDevice::getDeviceCount() == 0) {
        GTEST_SKIP() &lt;&lt; "No Kinect devices connected, skipping test";
    }
    
    KinectDevice device;
    DeviceConfig config;
    config.enableRGB = true;
    config.enableDepth = true;
    
    DeviceError result = device.initialize(config);
    EXPECT_EQ(result, DeviceError::None);
    EXPECT_TRUE(device.isInitialized());
}

TEST(KinectDeviceTest, StartStopStreams) {
    // Only run this test if devices are available
    if (KinectDevice::getDeviceCount() == 0) {
        GTEST_SKIP() &lt;&lt; "No Kinect devices connected, skipping test";
    }
    
    KinectDevice device;
    DeviceConfig config;
    config.enableRGB = true;
    config.enableDepth = true;
    
    DeviceError result = device.initialize(config);
    ASSERT_EQ(result, DeviceError::None);
    
    result = device.startStreams();
    EXPECT_EQ(result, DeviceError::None);
    EXPECT_TRUE(device.isStreaming());
    
    // Wait a bit to collect some frames
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    result = device.stopStreams();
    EXPECT_EQ(result, DeviceError::None);
    EXPECT_FALSE(device.isStreaming());
}

TEST(KinectDeviceTest, MotorControl) {
    // Only run this test if devices are available
    if (KinectDevice::getDeviceCount() == 0) {
        GTEST_SKIP() &lt;&lt; "No Kinect devices connected, skipping test";
    }
    
    KinectDevice device;
    DeviceConfig config;
    config.enableMotor = true;
    
    DeviceError result = device.initialize(config);
    ASSERT_EQ(result, DeviceError::None);
    
    // Test setting tilt angle
    result = device.setTiltAngle(15.0);
    EXPECT_EQ(result, DeviceError::None);
    
    // Wait for motor to move
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Test getting current angle
    double angle;
    result = device.getCurrentTiltAngle(angle);
    EXPECT_EQ(result, DeviceError::None);
    std::cout &lt;&lt; "Current angle: " &lt;&lt; angle &lt;&lt; " degrees" &lt;&lt; std::endl;
    
    // Reset to 0
    result = device.resetTiltAngle();
    EXPECT_EQ(result, DeviceError::None);
}

// Main entry point
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&amp;argc, argv);
    return RUN_ALL_TESTS();
}
```


## Example Applications

### Device Info Example (examples/device_info.cpp)

```cpp
#include "kinect_xr/device.h"
#include &lt;iostream&gt;

using namespace kinect_xr;

int main() {
    std::cout &lt;&lt; "Kinect Device Information Example" &lt;&lt; std::endl;
    std::cout &lt;&lt; "--------------------------------" &lt;&lt; std::endl;
    
    // Get count of connected devices
    int deviceCount = KinectDevice::getDeviceCount();
    std::cout &lt;&lt; "Found " &lt;&lt; deviceCount &lt;&lt; " Kinect devices" &lt;&lt; std::endl;
    
    if (deviceCount == 0) {
        std::cout &lt;&lt; "No devices found. Please connect a Kinect device." &lt;&lt; std::endl;
        return 0;
    }
    
    // Get information about all connected devices
    auto devices = getConnectedDevices();
    
    for (const auto&amp; info : devices) {
        std::cout &lt;&lt; "\nDevice ID: " &lt;&lt; info.id &lt;&lt; std::endl;
        std::cout &lt;&lt; "Serial Number: " &lt;&lt; 
            (info.serialNumber.empty() ? "Unknown" : info.serialNumber) &lt;&lt; std::endl;
        std::cout &lt;&lt; "Cameras: " &lt;&lt; info.cameraCount &lt;&lt; std::endl;
        std::cout &lt;&lt; "Has Motor: " &lt;&lt; (info.hasMotor ? "Yes" : "No") &lt;&lt; std::endl;
        std::cout &lt;&lt; "Has Accelerometer: " &lt;&lt; (info.hasAccelerometer ? "Yes" : "No") &lt;&lt; std::endl;
        std::cout &lt;&lt; "Has LED: " &lt;&lt; (info.hasLED ? "Yes" : "No") &lt;&lt; std::endl;
        
        // Try to get more detailed information by initializing the device
        KinectDevice device(info.id);
        DeviceConfig config;
        if (device.initialize(config) == DeviceError::None) {
            // Get motor angle if available
            if (info.hasMotor) {
                double angle;
                if (device.getCurrentTiltAngle(angle) == DeviceError::None) {
                    std::cout &lt;&lt; "Current Tilt Angle: " &lt;&lt; angle &lt;&lt; " degrees" &lt;&lt; std::endl;
                }
            }
        }
    }
    
    return 0;
}
```


### Stream Viewer Example (examples/stream_viewer.cpp)

```cpp
#include "kinect_xr/device.h"
#include &lt;iostream&gt;
#include &lt;thread&gt;
#include &lt;chrono&gt;
#include &lt;atomic&gt;
#include &lt;iomanip&gt;

#ifdef USE_OPENCV
#include &lt;opencv2/opencv.hpp&gt;
#endif

using namespace kinect_xr;

// Flag to control the main loop
std::atomic&lt;bool&gt; running(true);

// Frame counters
std::atomic&lt;int&gt; depthFrameCount(0);
std::atomic&lt;int&gt; rgbFrameCount(0);

// Last frame timestamps
std::atomic&lt;uint32_t&gt; lastDepthTimestamp(0);
std::atomic&lt;uint32_t&gt; lastRgbTimestamp(0);

// Function to handle depth frames
void onDepthFrame(const std::vector&lt;uint16_t&gt;&amp; depthData) {
    depthFrameCount++;
    
#ifdef USE_OPENCV
    // Create an image to display depth data
    int width = 640;  // Assuming VGA resolution
    int height = 480;
    
    // Create OpenCV matrix for visualization
    cv::Mat depthImg(height, width, CV_8UC1);
    
    // Convert depth data to a visible range (0-255)
    for (int y = 0; y &lt; height; y++) {
        for (int x = 0; x &lt; width; x++) {
            int index = y * width + x;
            uint16_t depth = depthData[index];
            
            // Convert to visible range (0-255)
            // Kinect values typically range from 0 to 2047
            uint8_t mappedDepth = static_cast&lt;uint8_t&gt;(std::min(255.0, depth / 8.0));
            
            depthImg.at&lt;uint8_t&gt;(y, x) = mappedDepth;
        }
    }
    
    // Apply colormap for better visualization
    cv::Mat colorDepth;
    cv::applyColorMap(depthImg, colorDepth, cv::COLORMAP_JET);
    
    // Display the depth image
    cv::imshow("Depth Stream", colorDepth);
    cv::waitKey(1);
#endif
}

// Function to handle RGB frames
void onRgbFrame(const std::vector&lt;uint8_t&gt;&amp; rgbData) {
    rgbFrameCount++;
    
#ifdef USE_OPENCV
    // Create an image to display RGB data
    int width = 640;  // Assuming VGA resolution
    int height = 480;
    
    // Create OpenCV matrix for visualization
    cv::Mat rgbImg(height, width, CV_8UC3, const_cast&lt;uint8_t*&gt;(rgbData.data()));
    
    // Convert BGR to RGB (OpenCV uses BGR by default)
    cv::cvtColor(rgbImg, rgbImg, cv::COLOR_RGB2BGR);
    
    // Display the RGB image
    cv::imshow("RGB Stream", rgbImg);
    cv::waitKey(1);
#endif
}

// Function to print frame stats
void printStats() {
    while (running) {
        std::cout &lt;&lt; "\rDepth FPS: " &lt;&lt; depthFrameCount &lt;&lt; " | RGB FPS: " &lt;&lt; rgbFrameCount &lt;&lt; "       " &lt;&lt; std::flush;
        
        // Reset counters
        depthFrameCount = 0;
        rgbFrameCount = 0;
        
        // Wait a second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    std::cout &lt;&lt; "Kinect Stream Viewer Example" &lt;&lt; std::endl;
    std::cout &lt;&lt; "-------------------------" &lt;&lt; std::endl;
    
    // Check if devices are available
    int deviceCount = KinectDevice::getDeviceCount();
    if (deviceCount == 0) {
        std::cout &lt;&lt; "No Kinect devices found. Please connect a Kinect and try again." &lt;&lt; std::endl;
        return 1;
    }
    
    // Initialize device
    KinectDevice device;
    DeviceConfig config;
    config.enableRGB = true;
    config.enableDepth = true;
    config.rgbResolution = 1;  // VGA
    config.depthResolution = 1;  // VGA
    
    // Initialize device
    DeviceError result = device.initialize(config);
    if (result != DeviceError::None) {
        std::cout &lt;&lt; "Failed to initialize Kinect: " &lt;&lt; errorToString(result) &lt;&lt; std::endl;
        return 1;
    }
    
    // Register callbacks
    device.setDepthCallback(onDepthFrame);
    device.setRgbCallback(onRgbFrame);
    
    // Start stats thread
    std::thread statsThread(printStats);
    
    // Start streams
    result = device.startStreams();
    if (result != DeviceError::None) {
        std::cout &lt;&lt; "Failed to start streams: " &lt;&lt; errorToString(result) &lt;&lt; std::endl;
        running = false;
        statsThread.join();
        return 1;
    }
    
    std::cout &lt;&lt; "Streams started successfully. Press Ctrl+C to exit." &lt;&lt; std::endl;
    
    // Wait for user to press a key
    try {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
#ifdef USE_OPENCV
            // Check for key press in OpenCV window
            int key = cv::waitKey(1);
            if (key == 27) {  // ESC key
                running = false;
            }
#endif
        }
    } catch (const std::exception&amp; e) {
        std::cout &lt;&lt; "Error: " &lt;&lt; e.what() &lt;&lt; std::endl;
    } catch (...) {
        // Catch any signals or interrupts
    }
    
    // Stop streams
    device.stopStreams();
    
    // Wait for stats thread to finish
    running = false;
    statsThread.join();
    
    std::cout &lt;&lt; "\nStreams stopped." &lt;&lt; std::endl;
    
    return 0;
}
```


## Building and Running

### Build Instructions

```bash
# Create build directory
mkdir -p build &amp;&amp; cd build

# Configure with CMake
cmake ..

# Build
make -j$(sysctl -n hw.ncpu)

# Run tests
ctest

# Run examples
./bin/device_info
./bin/stream_viewer
```


## USB Permissions on macOS

On macOS, you might need special permissions to access USB devices:

1. Disconnect the Kinect
2. Install the libfreenect driver from Homebrew
3. Reconnect the Kinect
4. If prompted, approve USB device access

If you encounter permission issues, try:

```bash
# For temporary access
sudo chmod +rw /dev/usb*

# For a more permanent solution, create a udev rule (requires admin privileges)
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="045e", ATTRS{idProduct}=="02b0", MODE="0666"' | sudo tee /etc/udev/rules.d/51-kinect.rules
```


## Validation and Troubleshooting

To verify successful implementation of Phase 1:

1. Run the device_info example to confirm device detection
2. Run the stream_viewer example to verify RGB and depth streams
3. Run tests to validate core functionality

Common issues and solutions:

- **Device not found**: Check USB connections and permissions
- **Initialization fails**: Verify libfreenect installation and device drivers
- **Streams don't start**: Check for conflicting applications using the Kinect
- **Build errors**: Ensure all dependencies are properly installed

With this implementation complete, you now have the foundation for your Kinect XR project. The next phase will build upon this to implement the OpenXR runtime functionality.

<div style="text-align: center">⁂</div>

[^1]: https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/collection_45c63562-3ed2-4f84-9224-8b3645afa9b3/dd3c4aa8-f3f5-4922-b7c0-c541c57bdc2f/Kinect-XR-Implementation-Plan.md

[^2]: https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/collection_45c63562-3ed2-4f84-9224-8b3645afa9b3/4096a457-8b65-40cd-98bd-dd883d9e61a8/Kinect-XR-Project-Summary.md

[^3]: https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/collection_45c63562-3ed2-4f84-9224-8b3645afa9b3/b1b27abb-2679-4e14-8802-1b4958fc8001/Kinect-XR-Research.md

