# Phase 1: Development Environment \& Kinect Integration Guide for macOS

This guide provides a comprehensive approach to setting up the Kinect XR development environment on macOS, focusing on Phase 1 of the implementation plan which establishes the foundation for the entire project.

## Setting Up the Development Environment

### Understanding the Toolchain Requirements

Before installing any software, it's important to understand the core tools needed for this project:

```bash
# Install essential development tools via Homebrew
brew install cmake git clang-format doxygen ninja
```

Each tool serves a specific purpose in our development workflow:

- **CMake**: Cross-platform build system that will manage our project dependencies and build configuration[^1][^7]
- **Git**: Version control system for tracking code changes
- **clang-format**: Code formatting tool to maintain consistent code style
- **Doxygen**: Documentation generator for C++ code
- **Ninja**: High-performance build system that works well with CMake

Apple's development tools are also required:

```bash
# Ensure Xcode Command Line Tools are installed
xcode-select --install
```

This installs the basic compiler toolchain needed for C++ development on macOS without requiring the full Xcode application[^5][^6].

### Project Directory Structure

Creating a well-organized directory structure is crucial for maintainability:

```bash
# Create project directory with standard C++ project layout
mkdir -p kinect-xr/{src,include,extern,tests,build,docs}
cd kinect-xr
```

This structure follows C++ best practices:

- `src`: Contains implementation files (.cpp)
- `include`: Header files (.h)
- `extern`: Third-party dependencies
- `tests`: Unit and integration tests
- `build`: Build artifacts (excluded from version control)
- `docs`: Project documentation


### CMake Configuration Setup

CMake is used to manage our build process in a platform-independent way. Create a root `CMakeLists.txt` file:

```cmake
cmake_minimum_required(VERSION 3.20)
project(KinectXR VERSION 0.1.0 LANGUAGES CXX)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# macOS-specific settings
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15") # Minimum macOS Catalina
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
    # For Apple Silicon
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    # For Intel Macs
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
endif()

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find libfreenect
find_package(libfreenect REQUIRED)

# Enable testing
enable_testing()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)
```

This configuration:

- Sets C++17 as our standard for modern language features
- Configures macOS-specific options with architecture detection for both Intel and Apple Silicon processors
- Organizes build outputs in standard locations
- Requires the libfreenect package
- Enables testing through CTest (CMake's testing framework)


### Creating a CMake Presets File

To simplify build configurations, create a `CMakePresets.json` file:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "debug",
      "displayName": "Debug",
      "description": "Debug build with symbols",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    },
    {
      "name": "release",
      "displayName": "Release",
      "description": "Optimized release build",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "debug",
      "configurePreset": "debug"
    },
    {
      "name": "release",
      "configurePreset": "release"
    }
  ]
}
```

This creates predefined configurations that can be easily selected without typing complex command arguments[^7].

## libfreenect Integration

### Understanding the libfreenect Library

libfreenect is an open-source library that provides access to the Microsoft Kinect sensor. According to the project summary, libfreenect has already been tested on macOS[^3], but we'll need to set up our development environment with it properly.

### Installation Options

There are two approaches to integrating libfreenect:

**Option 1: Install via Homebrew (recommended for initial setup)**

```bash
# Install libfreenect through Homebrew
brew install libfreenect
```

Using Homebrew simplifies installation and ensures compatibility with macOS. This approach is suitable for initial development and testing.

**Option 2: Build from source (for development and customization)**

Building from source gives us more control and allows modifications if needed:

```bash
# Clone libfreenect repository
git clone https://github.com/OpenKinect/libfreenect.git extern/libfreenect

# Build libfreenect
cd extern/libfreenect
mkdir build &amp;&amp; cd build
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_EXAMPLES=ON \
      -DBUILD_FAKENECT=ON \
      -DCMAKE_INSTALL_PREFIX=../../../install \
      ..
make
make install
cd ../../..
```

This approach:

- Places libfreenect source in our project's `extern` directory
- Builds with examples and the "fakenect" tool for testing without a physical device
- Installs to a local directory within our project for isolation
- Gives us the flexibility to modify the library if needed during development


### Testing the Kinect Connection

Before proceeding, verify that libfreenect can communicate with your Kinect:

```bash
# Test Kinect detection and basic functionality
freenect-glview
```

This command should open a window showing the RGB and depth feeds from your Kinect. If it doesn't work, check USB connections and permissions[^1].

## Implementing Kinect Device Management

### Creating the Device Manager Interface

Now let's implement the KinectDevice class as outlined in the implementation plan[^1]. First, create a header file:

```bash
# Create the header file
mkdir -p include/kinect-xr
touch include/kinect-xr/KinectDevice.h
```

In `include/kinect-xr/KinectDevice.h`, implement the interface:

```cpp
/**
 * @file KinectDevice.h
 * @brief Abstraction layer over libfreenect for Kinect device management
 */

#pragma once

#include &lt;libfreenect/libfreenect.h&gt;
#include &lt;memory&gt;
#include &lt;vector&gt;
#include &lt;mutex&gt;
#include &lt;functional&gt;

namespace kinect_xr {

/**
 * @class KinectDevice
 * @brief Manages connection and data streams from a Kinect device
 * 
 * This class provides a C++ abstraction over the libfreenect C API,
 * handling device discovery, initialization, and data streaming.
 */
class KinectDevice {
public:
    /**
     * @brief Construct a new Kinect Device object
     */
    KinectDevice();
    
    /**
     * @brief Destroy the Kinect Device object and release resources
     */
    ~KinectDevice();
    
    /**
     * @brief Initialize the Kinect device
     * 
     * @return true if initialization successful
     * @return false if no devices found or initialization failed
     */
    bool initialize();
    
    /**
     * @brief Start RGB and depth data streams
     * 
     * @return true if streams started successfully
     * @return false if streams failed to start
     */
    bool startStreams();
    
    /**
     * @brief Stop active data streams
     * 
     * @return true if streams stopped successfully
     * @return false if streams failed to stop
     */
    bool stopStreams();
    
    /**
     * @brief Set the tilt angle of the Kinect's motor
     * 
     * @param angleInDegrees The angle in degrees (-31 to 31)
     * @return true if angle was set successfully
     * @return false if angle could not be set
     */
    bool setTiltAngle(double angleInDegrees);
    
    /**
     * @brief Get the current tilt angle
     * 
     * @return double Current tilt angle in degrees
     */
    double getCurrentTiltAngle();
    
    /**
     * @brief Reset tilt angle to zero position
     * 
     * @return true if reset was successful
     * @return false if reset failed
     */
    bool resetTiltAngle();
    
    /**
     * @brief Get depth data from the most recent frame
     * 
     * @return const std::vector&lt;uint16_t&gt;&amp; Reference to depth buffer
     */
    const std::vector&lt;uint16_t&gt;&amp; getDepthData() const;
    
    /**
     * @brief Get RGB data from the most recent frame
     * 
     * @return const std::vector&lt;uint8_t&gt;&amp; Reference to RGB buffer
     */
    const std::vector&lt;uint8_t&gt;&amp; getRGBData() const;
    
    /**
     * @brief Register a callback for new depth frames
     * 
     * @param callback Function to call when new depth data arrives
     */
    void setDepthCallback(std::function&lt;void(const std::vector&lt;uint16_t&gt;&amp;)&gt; callback);
    
    /**
     * @brief Register a callback for new RGB frames
     * 
     * @param callback Function to call when new RGB data arrives
     */
    void setRGBCallback(std::function&lt;void(const std::vector&lt;uint8_t&gt;&amp;)&gt; callback);

private:
    // libfreenect context and device handles
    freenect_context* ctx;
    freenect_device* dev;
    
    // Data buffers
    std::vector&lt;uint16_t&gt; depthBuffer;
    std::vector&lt;uint8_t&gt; rgbBuffer;
    
    // Mutex for thread-safe access to buffers
    mutable std::mutex depthMutex;
    mutable std::mutex rgbMutex;
    
    // Streaming state
    bool isStreaming;
    
    // Callbacks
    std::function&lt;void(const std::vector&lt;uint16_t&gt;&amp;)&gt; depthCallback;
    std::function&lt;void(const std::vector&lt;uint8_t&gt;&amp;)&gt; rgbCallback;
    
    // Static callback functions for libfreenect
    static void depthCallbackWrapper(freenect_device* dev, void* depth, uint32_t timestamp);
    static void rgbCallbackWrapper(freenect_device* dev, void* rgb, uint32_t timestamp);
    
    // Instance map for callbacks
    static std::map&lt;freenect_device*, KinectDevice*&gt; deviceMap;
};

} // namespace kinect_xr
```

This header:

- Follows the class structure outlined in the implementation plan[^1]
- Adds proper documentation using Doxygen syntax
- Includes thread-safety with mutexes for data access
- Provides callback mechanisms for real-time data processing
- Uses a static mapping to handle C callbacks in an object-oriented way


### Implementing the Device Manager

Now create the implementation file:

```bash
# Create the implementation file
mkdir -p src/kinect
touch src/kinect/KinectDevice.cpp
```

In `src/kinect/KinectDevice.cpp`:

```cpp
/**
 * @file KinectDevice.cpp
 * @brief Implementation of the KinectDevice class
 */

#include "kinect-xr/KinectDevice.h"
#include &lt;iostream&gt;
#include &lt;cstring&gt;

namespace kinect_xr {

// Initialize static device map
std::map&lt;freenect_device*, KinectDevice*&gt; KinectDevice::deviceMap;

KinectDevice::KinectDevice() 
    : ctx(nullptr), dev(nullptr), isStreaming(false) {
    
    // Initialize buffers with appropriate sizes
    // Kinect depth is 640x480 11-bit values
    depthBuffer.resize(640 * 480);
    
    // RGB is 640x480 with 3 bytes per pixel
    rgbBuffer.resize(640 * 480 * 3);
}

KinectDevice::~KinectDevice() {
    // Ensure streams are stopped and device is closed
    if (isStreaming) {
        stopStreams();
    }
    
    if (dev) {
        freenect_close_device(dev);
        deviceMap.erase(dev);
    }
    
    if (ctx) {
        freenect_shutdown(ctx);
    }
}

bool KinectDevice::initialize() {
    // Initialize libfreenect context
    if (freenect_init(&amp;ctx, nullptr) &lt; 0) {
        std::cerr &lt;&lt; "Failed to initialize libfreenect" &lt;&lt; std::endl;
        return false;
    }
    
    // Set log level
    freenect_set_log_level(ctx, FREENECT_LOG_WARNING);
    
    // Select subdevices (camera and motor)
    freenect_select_subdevices(ctx, 
        static_cast&lt;freenect_device_flags&gt;(FREENECT_DEVICE_CAMERA | FREENECT_DEVICE_MOTOR));
    
    // Check for connected devices
    int num_devices = freenect_num_devices(ctx);
    if (num_devices &lt; 1) {
        std::cerr &lt;&lt; "No Kinect devices found" &lt;&lt; std::endl;
        freenect_shutdown(ctx);
        ctx = nullptr;
        return false;
    }
    
    // Open the first device
    if (freenect_open_device(ctx, &amp;dev, 0) &lt; 0) {
        std::cerr &lt;&lt; "Could not open Kinect device" &lt;&lt; std::endl;
        freenect_shutdown(ctx);
        ctx = nullptr;
        return false;
    }
    
    // Store this instance in the device map for callbacks
    deviceMap[dev] = this;
    
    std::cout &lt;&lt; "Kinect device initialized successfully" &lt;&lt; std::endl;
    return true;
}

bool KinectDevice::startStreams() {
    if (!dev) {
        std::cerr &lt;&lt; "Device not initialized" &lt;&lt; std::endl;
        return false;
    }
    
    if (isStreaming) {
        std::cerr &lt;&lt; "Streams already started" &lt;&lt; std::endl;
        return true; // Not an error, already running
    }
    
    // Set callback functions
    freenect_set_depth_callback(dev, depthCallbackWrapper);
    freenect_set_video_callback(dev, rgbCallbackWrapper);
    
    // Set depth format (11-bit packed data)
    if (freenect_set_depth_mode(dev, freenect_find_depth_mode(
            FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT)) != 0) {
        std::cerr &lt;&lt; "Failed to set depth mode" &lt;&lt; std::endl;
        return false;
    }
    
    // Set video format (RGB)
    if (freenect_set_video_mode(dev, freenect_find_video_mode(
            FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB)) != 0) {
        std::cerr &lt;&lt; "Failed to set video mode" &lt;&lt; std::endl;
        return false;
    }
    
    // Start depth stream
    if (freenect_start_depth(dev) != 0) {
        std::cerr &lt;&lt; "Failed to start depth stream" &lt;&lt; std::endl;
        return false;
    }
    
    // Start video stream
    if (freenect_start_video(dev) != 0) {
        std::cerr &lt;&lt; "Failed to start video stream" &lt;&lt; std::endl;
        freenect_stop_depth(dev);
        return false;
    }
    
    isStreaming = true;
    std::cout &lt;&lt; "Kinect streams started" &lt;&lt; std::endl;
    
    return true;
}

bool KinectDevice::stopStreams() {
    if (!dev || !isStreaming) {
        return false;
    }
    
    // Stop streams
    freenect_stop_video(dev);
    freenect_stop_depth(dev);
    
    isStreaming = false;
    std::cout &lt;&lt; "Kinect streams stopped" &lt;&lt; std::endl;
    
    return true;
}

bool KinectDevice::setTiltAngle(double angleInDegrees) {
    if (!dev) {
        return false;
    }
    
    // Clamp angle to valid range (-31 to 31 degrees)
    double clampedAngle = std::max(-31.0, std::min(31.0, angleInDegrees));
    
    // Convert to raw value and set
    if (freenect_set_tilt_degs(dev, clampedAngle) != 0) {
        std::cerr &lt;&lt; "Failed to set tilt angle" &lt;&lt; std::endl;
        return false;
    }
    
    return true;
}

double KinectDevice::getCurrentTiltAngle() {
    if (!dev) {
        return 0.0;
    }
    
    freenect_raw_tilt_state* state = nullptr;
    freenect_update_tilt_state(dev);
    state = freenect_get_tilt_state(dev);
    
    if (!state) {
        std::cerr &lt;&lt; "Failed to get tilt state" &lt;&lt; std::endl;
        return 0.0;
    }
    
    return freenect_get_tilt_degs(state);
}

bool KinectDevice::resetTiltAngle() {
    return setTiltAngle(0.0);
}

const std::vector&lt;uint16_t&gt;&amp; KinectDevice::getDepthData() const {
    std::lock_guard&lt;std::mutex&gt; lock(depthMutex);
    return depthBuffer;
}

const std::vector&lt;uint8_t&gt;&amp; KinectDevice::getRGBData() const {
    std::lock_guard&lt;std::mutex&gt; lock(rgbMutex);
    return rgbBuffer;
}

void KinectDevice::setDepthCallback(std::function&lt;void(const std::vector&lt;uint16_t&gt;&amp;)&gt; callback) {
    depthCallback = callback;
}

void KinectDevice::setRGBCallback(std::function&lt;void(const std::vector&lt;uint8_t&gt;&amp;)&gt; callback) {
    rgbCallback = callback;
}

// Static callback handlers
void KinectDevice::depthCallbackWrapper(freenect_device* dev, void* depth, uint32_t timestamp) {
    auto it = deviceMap.find(dev);
    if (it != deviceMap.end()) {
        KinectDevice* device = it-&gt;second;
        
        // Lock the depth buffer
        std::lock_guard&lt;std::mutex&gt; lock(device-&gt;depthMutex);
        
        // Copy data into our buffer
        // Note: depth buffer is 11-bit, so each pixel is 2 bytes
        std::memcpy(device-&gt;depthBuffer.data(), depth, device-&gt;depthBuffer.size() * sizeof(uint16_t));
        
        // Call user callback if registered
        if (device-&gt;depthCallback) {
            device-&gt;depthCallback(device-&gt;depthBuffer);
        }
    }
}

void KinectDevice::rgbCallbackWrapper(freenect_device* dev, void* rgb, uint32_t timestamp) {
    auto it = deviceMap.find(dev);
    if (it != deviceMap.end()) {
        KinectDevice* device = it-&gt;second;
        
        // Lock the RGB buffer
        std::lock_guard&lt;std::mutex&gt; lock(device-&gt;rgbMutex);
        
        // Copy data into our buffer
        std::memcpy(device-&gt;rgbBuffer.data(), rgb, device-&gt;rgbBuffer.size());
        
        // Call user callback if registered
        if (device-&gt;rgbCallback) {
            device-&gt;rgbCallback(device-&gt;rgbBuffer);
        }
    }
}

} // namespace kinect_xr
```

This implementation:

- Provides a complete C++ wrapper around the libfreenect C API
- Handles device initialization, stream management, and resource cleanup
- Uses proper error handling and reporting
- Implements thread-safe data access through mutexes
- Supports callback registration for event-driven processing
- Maps C callbacks to C++ methods using a static device map


### Setting Up the CMake Configuration for Source Files

Create a CMake configuration file for the source directory:

```bash
# Create CMakeLists.txt for src directory
touch src/CMakeLists.txt
```

In `src/CMakeLists.txt`:

```cmake
# Add library target
add_library(kinect_xr
    kinect/KinectDevice.cpp
)

# Set include directories
target_include_directories(kinect_xr
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${libfreenect_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(kinect_xr
    PRIVATE
        ${libfreenect_LIBRARIES}
)

# Install targets
install(TARGETS kinect_xr
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION include
)
```


## Setting Up the Testing Harness

Testing is critical for ensuring our implementation works correctly. Let's set up a comprehensive testing framework.

### Google Test Integration

First, let's add Google Test (gtest) as a dependency:

```bash
# Create external projects CMake file
touch extern/CMakeLists.txt
```

In `extern/CMakeLists.txt`:

```cmake
# Fetch and build Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(googletest)
```

Update the root `CMakeLists.txt` to include this:

```cmake
# Add after enable_testing()
add_subdirectory(extern)
```


### Creating the Kinect Device Tests

Now let's create a test for our Kinect device implementation:

```bash
# Create test directory and file
mkdir -p tests/kinect
touch tests/kinect/KinectDeviceTest.cpp
```

In `tests/kinect/KinectDeviceTest.cpp`:

```cpp
/**
 * @file KinectDeviceTest.cpp
 * @brief Unit tests for the KinectDevice class
 */

#include &lt;gtest/gtest.h&gt;
#include "kinect-xr/KinectDevice.h"
#include &lt;chrono&gt;
#include &lt;thread&gt;

using namespace kinect_xr;

class KinectDeviceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Nothing to set up
    }

    void TearDown() override {
        // Nothing to tear down
    }
};

// Test device initialization
TEST_F(KinectDeviceTest, Initialization) {
    KinectDevice device;
    
    // This test will be skipped if no physical device is connected
    if (freenect_init(nullptr, nullptr) &lt; 0 || freenect_num_devices(nullptr) &lt; 1) {
        GTEST_SKIP() &lt;&lt; "No Kinect device connected, skipping test";
    }
    
    EXPECT_TRUE(device.initialize());
}

// Test starting and stopping streams
TEST_F(KinectDeviceTest, StreamControl) {
    KinectDevice device;
    
    // Skip if no device
    if (!device.initialize()) {
        GTEST_SKIP() &lt;&lt; "Could not initialize Kinect device, skipping test";
    }
    
    // Test starting streams
    EXPECT_TRUE(device.startStreams());
    
    // Wait for some frames to be captured
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test stopping streams
    EXPECT_TRUE(device.stopStreams());
}

// Test data acquisition
TEST_F(KinectDeviceTest, DataAcquisition) {
    KinectDevice device;
    
    // Skip if no device
    if (!device.initialize()) {
        GTEST_SKIP() &lt;&lt; "Could not initialize Kinect device, skipping test";
    }
    
    // Start streams
    ASSERT_TRUE(device.startStreams());
    
    // Wait for frames to be captured
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Check depth data
    auto depthData = device.getDepthData();
    EXPECT_EQ(depthData.size(), 640 * 480);
    
    // Check RGB data
    auto rgbData = device.getRGBData();
    EXPECT_EQ(rgbData.size(), 640 * 480 * 3);
    
    // Stop streams
    device.stopStreams();
}

// Test tilt control
TEST_F(KinectDeviceTest, TiltControl) {
    KinectDevice device;
    
    // Skip if no device
    if (!device.initialize()) {
        GTEST_SKIP() &lt;&lt; "Could not initialize Kinect device, skipping test";
    }
    
    // Test setting tilt
    EXPECT_TRUE(device.setTiltAngle(15.0));
    
    // Give motor time to move
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Check current angle (allowing for some error in precision)
    double angle = device.getCurrentTiltAngle();
    EXPECT_NEAR(angle, 15.0, 2.0);
    
    // Test resetting tilt
    EXPECT_TRUE(device.resetTiltAngle());
    
    // Give motor time to move
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Check reset angle
    angle = device.getCurrentTiltAngle();
    EXPECT_NEAR(angle, 0.0, 2.0);
}

// Test callbacks
TEST_F(KinectDeviceTest, Callbacks) {
    KinectDevice device;
    
    // Skip if no device
    if (!device.initialize()) {
        GTEST_SKIP() &lt;&lt; "Could not initialize Kinect device, skipping test";
    }
    
    bool depthCallbackTriggered = false;
    bool rgbCallbackTriggered = false;
    
    // Set callbacks
    device.setDepthCallback([&amp;depthCallbackTriggered](const std::vector&lt;uint16_t&gt;&amp; data) {
        depthCallbackTriggered = true;
    });
    
    device.setRGBCallback([&amp;rgbCallbackTriggered](const std::vector&lt;uint8_t&gt;&amp; data) {
        rgbCallbackTriggered = true;
    });
    
    // Start streams
    ASSERT_TRUE(device.startStreams());
    
    // Wait for callbacks to be triggered
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // Stop streams
    device.stopStreams();
    
    // Check if callbacks were triggered
    EXPECT_TRUE(depthCallbackTriggered);
    EXPECT_TRUE(rgbCallbackTriggered);
}
```

This testing framework:

- Creates comprehensive tests for all KinectDevice functionality
- Includes tests for initialization, streaming, data acquisition, tilt control, and callbacks
- Properly handles the case where no physical device is connected
- Uses appropriate wait times for hardware operations


### Test CMake Configuration

Create a CMake configuration file for tests:

```bash
# Create CMakeLists.txt for tests directory
touch tests/CMakeLists.txt
```

In `tests/CMakeLists.txt`:

```cmake
# Add test executable
add_executable(kinect_device_test
    kinect/KinectDeviceTest.cpp
)

# Link to our library and Google Test
target_link_libraries(kinect_device_test
    PRIVATE
        kinect_xr
        gtest
        gtest_main
)

# Add test to CTest
add_test(
    NAME KinectDeviceTest
    COMMAND kinect_device_test
)
```


## Setting Up Development Best Practices

### Code Style Configuration

Create a `.clang-format` file for consistent code style:

```bash
# Create clang-format configuration
cat &gt; .clang-format &lt;&lt; 'EOF'
---
Language: Cpp
BasedOnStyle: Google
AccessModifierOffset: -4
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: false
AlignConsecutiveDeclarations: false
AlignEscapedNewlines: Left
AlignOperands: true
AlignTrailingComments: true
AllowAllParametersOfDeclarationOnNextLine: true
AllowShortBlocksOnASingleLine: false
AllowShortCaseLabelsOnASingleLine: false
AllowShortFunctionsOnASingleLine: Empty
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
AlwaysBreakAfterDefinitionReturnType: None
AlwaysBreakAfterReturnType: None
AlwaysBreakBeforeMultilineStrings: false
AlwaysBreakTemplateDeclarations: Yes
BinPackArguments: true
BinPackParameters: true
BreakBeforeBinaryOperators: None
BreakBeforeBraces: Attach
BreakBeforeInheritanceComma: false
BreakInheritanceList: BeforeColon
BreakBeforeTernaryOperators: true
BreakConstructorInitializersBeforeComma: false
BreakConstructorInitializers: BeforeColon
BreakAfterJavaFieldAnnotations: false
BreakStringLiterals: true
ColumnLimit: 100
CommentPragmas: '^ IWYU pragma:'
CompactNamespaces: false
ConstructorInitializerAllOnOneLineOrOnePerLine: true
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4
Cpp11BracedListStyle: true
DerivePointerAlignment: false
DisableFormat: false
ExperimentalAutoDetectBinPacking: false
FixNamespaceComments: true
ForEachMacros:
  - foreach
  - Q_FOREACH
  - BOOST_FOREACH
IncludeBlocks: Regroup
IncludeCategories:
  - Regex: '^&lt;[^/]*&gt;'
    Priority: 1
  - Regex: '^&lt;.*&gt;'
    Priority: 2
  - Regex: '^".*"'
    Priority: 3
IncludeIsMainRegex: '([-_](test|unittest))?$'
IndentCaseLabels: true
IndentPPDirectives: None
IndentWidth: 4
IndentWrappedFunctionNames: false
JavaScriptQuotes: Leave
JavaScriptWrapImports: true
KeepEmptyLinesAtTheStartOfBlocks: false
MacroBlockBegin: ''
MacroBlockEnd: ''
MaxEmptyLinesToKeep: 1
NamespaceIndentation: None
ObjCBinPackProtocolList: Never
ObjCBlockIndentWidth: 2
ObjCSpaceAfterProperty: false
ObjCSpaceBeforeProtocolList: true
PenaltyBreakAssignment: 2
PenaltyBreakBeforeFirstCallParameter: 1
PenaltyBreakComment: 300
PenaltyBreakFirstLessLess: 120
PenaltyBreakString: 1000
PenaltyBreakTemplateDeclaration: 10
PenaltyExcessCharacter: 1000000
PenaltyReturnTypeOnItsOwnLine: 200
PointerAlignment: Left
RawStringFormats:
  - Language: Cpp
    Delimiters:
      - cc
      - CC
      - cpp
      - Cpp
      - CPP
      - 'c++'
      - 'C++'
    CanonicalDelimiter: ''
    BasedOnStyle: google
  - Language: TextProto
    Delimiters:
      - pb
      - PB
      - proto
      - PROTO
    EnclosingFunctions:
      - EqualsProto
      - EquivToProto
      - PARSE_PARTIAL_TEXT_PROTO
      - PARSE_TEST_PROTO
      - PARSE_TEXT_PROTO
      - ParseTextOrDie
      - ParseTextProtoOrDie
    CanonicalDelimiter: ''
    BasedOnStyle: google
ReflowComments: true
SortIncludes: true
SortUsingDeclarations: true
SpaceAfterCStyleCast: false
SpaceAfterTemplateKeyword: true
SpaceBeforeAssignmentOperators: true
SpaceBeforeCpp11BracedList: false
SpaceBeforeCtorInitializerColon: true
SpaceBeforeInheritanceColon: true
SpaceBeforeParens: ControlStatements
SpaceBeforeRangeBasedForLoopColon: true
SpaceInEmptyParentheses: false
SpacesBeforeTrailingComments: 2
SpacesInAngles: false
SpacesInContainerLiterals: false
SpacesInCStyleCastParentheses: false
SpacesInParentheses: false
SpacesInSquareBrackets: false
Standard: Auto
TabWidth: 4
UseTab: Never
EOF
```

This configuration follows the Google C++ style guide with some modifications for readability.

### Git Configuration

Set up appropriate Git ignore rules:

```bash
# Create .gitignore file
cat &gt; .gitignore &lt;&lt; 'EOF'
# Build directories
build/
install/

# macOS specific
.DS_Store
.AppleDouble
.LSOverride
._*

# IDE specific
.vscode/
.idea/
*.swp
*.swo
*~

# Compiled files
*.o
*.so
*.dylib
*.dSYM
*.a
*.lib
*.exe
*.out
*.app

# Debug files
*.dSYM/
*.su
*.idb
*.pdb

# Testing
*.gcda
*.gcno
*.gcov
EOF
```


### Creating a CI/CD Pipeline Configuration

GitHub Actions can be used for continuous integration. Create a workflow file:

```bash
# Create GitHub Actions workflow directory
mkdir -p .github/workflows
touch .github/workflows/ci.yml
```

In `.github/workflows/ci.yml`:

```yaml
name: CI

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  build:
    runs-on: macos-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        brew install cmake ninja libfreenect
    
    - name: Configure
      run: |
        cmake --preset=debug
    
    - name: Build
      run: |
        cmake --build --preset=debug
    
    - name: Test
      run: |
        cd build/debug
        ctest --output-on-failure
```

This workflow:

- Runs on macOS latest
- Installs required dependencies
- Configures the project in debug mode
- Builds using CMake presets
- Runs tests using CTest


## Phase 1 Validation Checklist

Before proceeding to Phase 2, verify that the following criteria are met:

### Development Environment

- [ ] CMake build system is configured and working
- [ ] Project compiles without warnings
- [ ] Development tools (clang-format, etc.) are properly set up
- [ ] Git version control is established with appropriate ignore rules


### Kinect Integration

- [ ] libfreenect is properly installed and accessible
- [ ] KinectDevice class initializes the Kinect successfully
- [ ] RGB and depth streams can be started and stopped
- [ ] Data from both streams can be accessed
- [ ] Motor control functions work properly


### Testing

- [ ] All unit tests pass
- [ ] Testing framework is established with Google Test
- [ ] Tests cover all critical functionality
- [ ] Tests handle the case when no device is connected


### Documentation

- [ ] Code includes Doxygen documentation
- [ ] README file is created with basic project information
- [ ] Project structure is well-organized and documented


## Conclusion

This completes the detailed guide for Phase 1 of the Kinect XR implementation on macOS. The foundation is now set for Phase 2, where we'll implement the OpenXR runtime. The Kinect device management system we've created provides a clean abstraction over libfreenect that will serve as the basis for integrating Kinect data with the OpenXR framework.

## References

1. Kinect XR Implementation Plan.md
2. Kinect XR Research.md
3. Kinect XR Project Summary.md
4. Stack Overflow: CMake failed building project on mac
5. Stack Overflow: How to use CMake on mac
6. LLVM.org: Getting Started: Building and Running Clang
7. GitHub: Halide - Building Halide With CMake

[^1]: https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/collection_45c63562-3ed2-4f84-9224-8b3645afa9b3/dd3c4aa8-f3f5-4922-b7c0-c541c57bdc2f/Kinect-XR-Implementation-Plan.md

[^2]: https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/collection_45c63562-3ed2-4f84-9224-8b3645afa9b3/b1b27abb-2679-4e14-8802-1b4958fc8001/Kinect-XR-Research.md

[^3]: https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/collection_45c63562-3ed2-4f84-9224-8b3645afa9b3/4096a457-8b65-40cd-98bd-dd883d9e61a8/Kinect-XR-Project-Summary.md

[^4]: https://stackoverflow.com/questions/24447322/cmake-failed-building-project-on-mac

[^5]: https://stackoverflow.com/questions/29727770/how-to-use-cmake-on-mac

[^6]: https://clang.llvm.org/get_started.html

[^7]: https://github.com/halide/Halide/blob/main/doc/BuildingHalideWithCMake.md

[^8]: https://github.com/ainfosec/ci_helloworld

[^9]: https://releases.llvm.org/14.0.0/docs/GettingStarted.html

[^10]: https://www.incredibuild.com/blog/how-to-set-up-new-projects-on-cmake-for-success

[^11]: https://github.com/OpenKinect/libfreenect/blob/master/README.md

[^12]: https://bornagainproject.org/git-main/dev/config/build/

[^13]: https://apple.stackexchange.com/questions/462338/how-to-properly-configure-cmake-tools-in-vscode-on-macos

[^14]: https://stackoverflow.com/questions/73108273/building-and-distributing-a-macos-application-using-cmake

[^15]: https://formulae.brew.sh/formula/libfreenect

[^16]: https://allpix-squared-forum.web.cern.ch/t/installation-error-mac-os-sonoma/561

[^17]: https://cmake.org/cmake/help/book/mastering-cmake/chapter/Getting Started.html

[^18]: https://en.sfml-dev.org/forums/index.php?topic=19550.0

[^19]: https://github.com/OpenKinect/libfreenect/issues/537

[^20]: https://www.reddit.com/r/cpp/comments/gvdu67/you_asked_i_listened_here_is_my_updated_and/

[^21]: https://www.reddit.com/r/vulkan/comments/m2tgnn/setup_cmake_file_on_macos/

[^22]: https://cmake.org/cmake/help/latest/manual/cmake.1.html

[^23]: https://fatokun.com/post/71953767569/are-you-having-trouble-installing-openkinect-on-your-mac

