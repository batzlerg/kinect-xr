# Kinect XR Tester's Guide

## What We Built

Kinect XR is now a **real OpenXR runtime** that can be discovered and used by any OpenXR application. Here's what that means:

### The OpenXR Ecosystem

```
┌─────────────────────────────────────────────────────────────────┐
│                     OpenXR Applications                         │
│  (Unity, Unreal, custom apps, hello_xr, OpenXR Explorer)       │
└─────────────────────────────┬───────────────────────────────────┘
                              │ Standard OpenXR API calls
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      OpenXR Loader                              │
│  (Reads XR_RUNTIME_JSON to find runtime)                       │
└─────────────────────────────┬───────────────────────────────────┘
                              │ Loads our .dylib
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Kinect XR Runtime  ◄── WE BUILT THIS          │
│  (libkinect_xr_runtime_lib.dylib)                              │
└─────────────────────────────┬───────────────────────────────────┘
                              │ (Future: connects to device layer)
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    KinectDevice Layer                           │
│  (Stream management, libfreenect integration)                   │
└─────────────────────────────────────────────────────────────────┘
```

### What Works Now

| Capability | Status | Description |
|------------|--------|-------------|
| Runtime Discovery | ✅ Working | OpenXR apps can find our runtime |
| Extension Enumeration | ✅ Working | Reports XR_KHR_composition_layer_depth support |
| Instance Creation | ✅ Working | Apps can create XrInstance handles |
| Instance Destruction | ✅ Working | Clean resource cleanup |
| Device Streaming | ✅ Working | Start/stop depth and RGB capture |

### What Doesn't Work Yet

| Capability | Status | Needed For |
|------------|--------|------------|
| Session Creation | ❌ Not implemented | Actual XR rendering |
| Swapchains | ❌ Not implemented | Frame submission |
| Reference Spaces | ❌ Not implemented | Spatial tracking |
| Frame Timing | ❌ Not implemented | Synchronized rendering |

**Bottom line:** Applications can discover us and create instances, but can't actually render anything yet. That's Phase 3+.

---

## Part 1: Software Testing (No Hardware Required)

### Prerequisites

```bash
# Ensure project is built
cd /Users/graham/projects/hardware/repos/kinect-xr
cmake -B build -S .
cmake --build build
```

### Test 1: Run Unit Tests

Validates the device layer and streaming state machine.

```bash
./build/bin/unit_tests
```

**Expected Output:**
```
[==========] Running 15 tests from 4 test suites.
[  PASSED  ] 11 tests.
[  SKIPPED ] 4 tests.  (hardware-dependent)
```

**What it tests:**
- libfreenect context initialization
- Device enumeration (returns 0 without hardware)
- Error string conversion
- Streaming state machine (initial state, error cases)

---

### Test 2: Run Runtime Tests

Validates the OpenXR runtime through the actual OpenXR loader.

```bash
./build/bin/runtime_tests
```

**Expected Output:**
```
[==========] Running 6 tests from 1 test suite.
[ RUN      ] RuntimeDiscoveryTest.EnumerateExtensions
[       OK ] RuntimeDiscoveryTest.EnumerateExtensions
[ RUN      ] RuntimeDiscoveryTest.EnumerateApiLayers
[       OK ] RuntimeDiscoveryTest.EnumerateApiLayers
[ RUN      ] RuntimeDiscoveryTest.CreateAndDestroyInstance
[       OK ] RuntimeDiscoveryTest.CreateAndDestroyInstance
[ RUN      ] RuntimeDiscoveryTest.CreateInstanceWithExtension
[       OK ] RuntimeDiscoveryTest.CreateInstanceWithExtension
[ RUN      ] RuntimeDiscoveryTest.CreateInstanceWithUnsupportedExtension
[       OK ] RuntimeDiscoveryTest.CreateInstanceWithUnsupportedExtension
[ RUN      ] RuntimeDiscoveryTest.DestroyInvalidInstance
[       OK ] RuntimeDiscoveryTest.DestroyInvalidInstance
[==========] 6 tests from 1 test suite ran.
[  PASSED  ] 6 tests.
```

**What it tests:**
- Our runtime is discovered via XR_RUNTIME_JSON
- Extension enumeration returns XR_KHR_composition_layer_depth
- Instance creation and destruction work correctly
- Error handling for unsupported extensions
- Invalid handle rejection

---

### Test 3: Manual Runtime Registration

Verify the runtime can be registered and discovered.

```bash
# Register our runtime
source scripts/register-runtime.sh

# Verify environment variable is set
echo $XR_RUNTIME_JSON
# Should print: /Users/graham/projects/hardware/repos/kinect-xr/manifest/kinect_xr.json

# Verify manifest exists and is valid JSON
cat $XR_RUNTIME_JSON
```

**Expected Output:**
```json
{
    "file_format_version": "1.0.0",
    "runtime": {
        "name": "Kinect XR Runtime",
        "library_path": "../build/lib/libkinect_xr_runtime_lib.dylib"
    }
}
```

---

### Test 4: Run OpenXR Spike Test

Validates OpenXR SDK integration (should "fail" - that's expected).

```bash
./build/bin/openxr_spike_test
```

**Expected Output:**
```
OpenXR SDK linked successfully
xrEnumerateInstanceExtensionProperties returned: -51
Extension count: 0
```

**Note:** Return code -51 is `XR_ERROR_RUNTIME_UNAVAILABLE`. This test does NOT use our runtime - it just validates the SDK links correctly. The "failure" is expected because the spike test doesn't set XR_RUNTIME_JSON.

---

### Test 5: Test With hello_xr (OpenXR SDK Sample)

The OpenXR SDK includes a sample application called `hello_xr`. Let's use it to test our runtime.

```bash
# Register our runtime first
source scripts/register-runtime.sh

# Run hello_xr (it will fail at session creation - expected!)
./build/_deps/openxr-build/src/tests/hello_xr/hello_xr -g Vulkan 2>&1 | head -20
```

**Expected Output:**
```
Using runtime: Kinect XR Runtime
Available extensions: 1
  XR_KHR_composition_layer_depth v1
Creating instance...
Instance created successfully
Getting instance properties...
Runtime: Kinect XR Runtime (version 0.1.0)
Creating session...
Error: Session creation not implemented  (or similar error)
```

**What this proves:**
- ✅ hello_xr found our runtime
- ✅ Extension enumeration worked
- ✅ Instance creation succeeded
- ❌ Session creation failed (expected - not implemented yet)

---

### Test 6: Write Your Own OpenXR Test

Create a simple C++ program that uses our runtime:

```cpp
// test_kinect_xr.cpp
#include <openxr/openxr.h>
#include <iostream>
#include <vector>

int main() {
    // Enumerate extensions
    uint32_t extCount = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);

    std::vector<XrExtensionProperties> extensions(extCount, {XR_TYPE_EXTENSION_PROPERTIES});
    xrEnumerateInstanceExtensionProperties(nullptr, extCount, &extCount, extensions.data());

    std::cout << "Extensions (" << extCount << "):" << std::endl;
    for (const auto& ext : extensions) {
        std::cout << "  " << ext.extensionName << " v" << ext.extensionVersion << std::endl;
    }

    // Create instance
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy(createInfo.applicationInfo.applicationName, "Test App");
    createInfo.applicationInfo.applicationVersion = 1;
    strcpy(createInfo.applicationInfo.engineName, "No Engine");
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrInstance instance;
    XrResult result = xrCreateInstance(&createInfo, &instance);

    if (result == XR_SUCCESS) {
        std::cout << "Instance created successfully!" << std::endl;

        // Get instance properties
        XrInstanceProperties props{XR_TYPE_INSTANCE_PROPERTIES};
        xrGetInstanceProperties(instance, &props);
        std::cout << "Runtime: " << props.runtimeName << std::endl;

        // Clean up
        xrDestroyInstance(instance);
        std::cout << "Instance destroyed." << std::endl;
    } else {
        std::cout << "Failed to create instance: " << result << std::endl;
    }

    return 0;
}
```

**To compile and run:**
```bash
# Set runtime
source scripts/register-runtime.sh

# Compile (adjust paths as needed)
clang++ -std=c++17 test_kinect_xr.cpp \
    -I./build/_deps/openxr-src/include \
    -L./build/_deps/openxr-build/src/loader \
    -lopenxr_loader \
    -o test_kinect_xr

# Run
./test_kinect_xr
```

---

### Test Summary: Software Only

| Test | Command | Expected Result |
|------|---------|-----------------|
| Unit Tests | `./build/bin/unit_tests` | 11 pass, 4 skip |
| Runtime Tests | `./build/bin/runtime_tests` | 6 pass |
| Registration | `source scripts/register-runtime.sh` | XR_RUNTIME_JSON set |
| hello_xr | Run with our runtime | Instance creates, session fails |

---

## Part 2: Hardware Testing (Kinect Required)

### Hardware Setup

1. **Connect Kinect to USB** (use USB 2.0 port or powered hub)
2. **Verify libfreenect can see it:**
   ```bash
   # If you have freenect-glview installed:
   freenect-glview

   # Or just run our tests:
   ./build/bin/unit_tests 2>&1 | grep "Found.*Kinect"
   ```

**Expected:** Should report "Found 1 Kinect device(s)"

---

### Test 7: Full Unit Tests With Hardware

```bash
./build/bin/unit_tests
```

**Expected Output (with hardware):**
```
[==========] Running 15 tests from 4 test suites.
[  PASSED  ] 15 tests.
[  SKIPPED ] 0 tests.
```

**What the 4 previously-skipped tests now verify:**
- `StartStreamsSetsStreamingFlag` - startStreams() works and sets flag
- `StartStreamsFailsWhenAlreadyStreaming` - duplicate start returns error
- `StopStreamsFailsWhenNotStreaming` - stop when not streaming returns error
- `StopStreamsClearsStreamingFlag` - stopStreams() works and clears flag

---

### Test 8: Integration Tests

```bash
./build/bin/integration_tests
```

**Expected Output:**
```
[==========] Running 3 tests from 2 test suites.
[ RUN      ] KinectHardwareTest.DetectKinectDevice
Found 1 Kinect device(s)
[       OK ] KinectHardwareTest.DetectKinectDevice
[ RUN      ] KinectHardwareTest.OpenKinectDevice
[       OK ] KinectHardwareTest.OpenKinectDevice
[ RUN      ] KinectDeviceTest.Initialize
[       OK ] KinectDeviceTest.Initialize
[==========] 3 tests from 2 test suites ran.
[  PASSED  ] 3 tests.
```

**What it tests:**
- Device detection via libfreenect
- Device opening (USB communication)
- KinectDevice initialization and configuration

---

### Test 9: Re-enable Stream Integration Test

The stream start/stop integration test is currently commented out. To fully validate with hardware:

1. Edit `tests/integration/device_integration_test.cpp`
2. Uncomment the `StartAndStopStreams` test (lines 77-104)
3. Rebuild and run:
   ```bash
   cmake --build build
   ./build/bin/integration_tests
   ```

**Expected:** Test should pass, verifying streams actually start and stop.

---

### Test 10: Manual Stream Test

Create a simple program to test streaming:

```cpp
// test_stream.cpp
#include "kinect_xr/device.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    kinect_xr::KinectDevice device;

    // Check for devices
    int count = kinect_xr::KinectDevice::getDeviceCount();
    std::cout << "Found " << count << " Kinect device(s)" << std::endl;

    if (count == 0) {
        std::cerr << "No Kinect connected!" << std::endl;
        return 1;
    }

    // Initialize
    kinect_xr::DeviceConfig config;
    config.enableRGB = true;
    config.enableDepth = true;

    auto result = device.initialize(config);
    if (result != kinect_xr::DeviceError::None) {
        std::cerr << "Init failed: " << kinect_xr::errorToString(result) << std::endl;
        return 1;
    }

    std::cout << "Device initialized" << std::endl;
    std::cout << "Starting streams..." << std::endl;

    result = device.startStreams();
    if (result != kinect_xr::DeviceError::None) {
        std::cerr << "Start failed: " << kinect_xr::errorToString(result) << std::endl;
        return 1;
    }

    std::cout << "Streaming: " << (device.isStreaming() ? "YES" : "NO") << std::endl;
    std::cout << "Running for 3 seconds..." << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "Stopping streams..." << std::endl;
    device.stopStreams();

    std::cout << "Streaming: " << (device.isStreaming() ? "YES" : "NO") << std::endl;
    std::cout << "Done!" << std::endl;

    return 0;
}
```

---

### Hardware Test Summary

| Test | Command | Expected Result |
|------|---------|-----------------|
| Device Detection | Unit tests | "Found 1 Kinect device(s)" |
| Unit Tests | `./build/bin/unit_tests` | 15 pass, 0 skip |
| Integration Tests | `./build/bin/integration_tests` | All pass |
| Stream Test | Custom program | Streams start/stop successfully |

---

## Troubleshooting

### "No Kinect devices found"

1. Check USB connection (try different port)
2. Verify libfreenect is installed: `brew list libfreenect`
3. Check USB permissions (may need to run as root first time)
4. Try `freenect-glview` to verify hardware works

### "Runtime not found" in OpenXR apps

1. Verify XR_RUNTIME_JSON is set: `echo $XR_RUNTIME_JSON`
2. Verify manifest exists: `cat $XR_RUNTIME_JSON`
3. Verify library exists: `ls -la build/lib/libkinect_xr_runtime_lib.dylib`
4. Run in same terminal where you sourced register-runtime.sh

### Build errors

```bash
# Clean rebuild
rm -rf build
cmake -B build -S .
cmake --build build
```

### Tests crash or hang

1. Check if another app is using Kinect
2. Unplug and replug Kinect
3. Check system logs: `log show --predicate 'process == "unit_tests"' --last 1m`

---

## What's Next?

After testing confirms everything works:

1. **Spec 004 (Metal Spike)** - Display Kinect video in a Metal window
2. **Session Management** - Implement xrCreateSession
3. **Swapchain Implementation** - Connect Kinect frames to OpenXR swapchains
4. **Full XR Support** - Reference spaces, frame timing, depth submission

The current implementation proves the architecture works. Next steps connect the device layer to the runtime layer and add actual rendering.
