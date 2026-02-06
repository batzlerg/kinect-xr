# Kinect XR - Architecture Document

## Overview

C++ OpenXR runtime layered on libfreenect for Kinect 1 hardware access. The architecture separates hardware abstraction from OpenXR API implementation, enabling clean testing and future extensibility.

## System Context

```
┌─────────────────────────────────────────────────────────────────┐
│                      Applications                                │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────────┐ │
│  │ Native OpenXR │  │ Unity/Unreal │  │ WebSocket Bridge      │ │
│  │ Applications  │  │ (OpenXR)     │  │ (Future - WebXR)      │ │
│  └──────┬───────┘  └──────┬───────┘  └──────────┬─────────────┘ │
└─────────┼─────────────────┼─────────────────────┼───────────────┘
          │                 │                     │
          └─────────────────┼─────────────────────┘
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Kinect XR Runtime                            │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  OpenXR API Layer (Phase 2+)                               │ │
│  │  - Instance/Session management                             │ │
│  │  - Swapchain handling                                      │ │
│  │  - Extension support (XR_KHR_composition_layer_depth)      │ │
│  └────────────────────────────────────────────────────────────┘ │
│                            │                                    │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Data Transformation Pipeline (Phase 4)                    │ │
│  │  - Depth format conversion (11-bit to OpenXR)              │ │
│  │  - Coordinate system mapping                               │ │
│  │  - Frame synchronization                                   │ │
│  └────────────────────────────────────────────────────────────┘ │
│                            │                                    │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Device Abstraction Layer (Phase 1) ◄── CURRENT            │ │
│  │  - KinectDevice class                                      │ │
│  │  - Stream management                                       │ │
│  │  - Configuration handling                                  │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                      libfreenect                                │
│  - USB isochronous transfer                                     │
│  - Raw depth/RGB frame capture                                  │
│  - Motor/LED control                                            │
└─────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Kinect Hardware                              │
│  - Depth camera (IR projector + CMOS sensor)                    │
│  - RGB camera                                                   │
│  - Motorized tilt                                               │
└─────────────────────────────────────────────────────────────────┘
```

## Components

### Device Abstraction Layer (Phase 1)

**Purpose:** Clean C++ wrapper around libfreenect C API.

**Files:**
- `include/kinect_xr/device.h` - Public interface
- `src/device.cpp` - Implementation

**Key Classes:**

```cpp
namespace kinect_xr {

enum class DeviceError { None, DeviceNotFound, InitializationFailed, Unknown };

struct DeviceConfig {
  bool enableRGB = true;
  bool enableDepth = true;
  bool enableMotor = true;
  int deviceId = 0;
};

class KinectDevice {
public:
  KinectDevice();
  ~KinectDevice();

  // Prevent copy (1:1 mapping to physical device)
  KinectDevice(const KinectDevice&) = delete;
  KinectDevice& operator=(const KinectDevice&) = delete;

  DeviceError initialize(const DeviceConfig& config);
  bool isInitialized() const;
  static int getDeviceCount();

  // Stream management (to be implemented)
  DeviceError startStreams();
  DeviceError stopStreams();
  bool isStreaming() const;

private:
  freenect_context* ctx_;
  freenect_device* dev_;
  bool initialized_;
  DeviceConfig config_;
};

}
```

**Design Decisions:**
- Forward-declare libfreenect types in header (minimal coupling)
- RAII for resource management
- Error codes over exceptions (hardware-oriented code)
- Delete copy operations (device identity)

### Data Transformation Pipeline (Phase 4)

**Purpose:** Convert Kinect data formats to OpenXR expectations.

**Planned Components:**

```cpp
class DepthTransformer {
  // Convert 11-bit Kinect depth to normalized float
  void transformDepthFrame(const uint16_t* src, float* dst,
                          uint32_t width, uint32_t height);

  // Map Kinect camera space to OpenXR reference space
  XrVector3f transformPoint(float x, float y, float depth);
};

class FrameSynchronizer {
  // Align depth and RGB frames by timestamp
  bool synchronize(const DepthFrame& depth, const RGBFrame& rgb);
};
```

### OpenXR API Layer (Phase 2)

**Purpose:** Standards-compliant OpenXR runtime implementation.

**Planned Structure:**

```cpp
class KinectXRRuntime {
  // Core OpenXR functions
  XrResult xrCreateInstance(...);
  XrResult xrDestroyInstance(...);
  XrResult xrCreateSession(...);
  XrResult xrBeginSession(...);
  XrResult xrEndSession(...);

  // Swapchain management
  XrResult xrCreateSwapchain(...);
  XrResult xrAcquireSwapchainImage(...);
  XrResult xrReleaseSwapchainImage(...);

  // Depth extension
  XrResult xrEndFrame(...);  // Handles depth layer submission

private:
  std::unique_ptr<KinectDevice> kinect_;
  std::unique_ptr<DepthTransformer> transformer_;
};
```

## Data Flow

### Current (Phase 1)

```
Kinect USB ─► libfreenect ─► KinectDevice ─► Test Application
                 │               │
                 │               └── Callback: void*(depth/rgb, timestamp)
                 │
                 └── Isochronous USB (30fps depth, 30fps RGB)
```

### Target (Phase 5)

```
Kinect USB ─► libfreenect ─► KinectDevice ─► DepthTransformer ─► KinectXRRuntime
                                                                       │
                                    ┌──────────────────────────────────┘
                                    │
                                    ▼
                             OpenXR Loader ─► OpenXR Applications
                                    │
                                    ▼
                             WebSocket Bridge ─► Browser (WebXR Polyfill)
```

## Key Decisions

| Decision | Rationale | Date | Alternatives Considered |
|----------|-----------|------|------------------------|
| libfreenect (user-provided) | Stable library, users may need custom builds | 2025-04 | Bundled via FetchContent |
| OpenXR over custom API | Industry standard, WebXR compatibility | 2025-04 | Direct WebSocket to browser |
| C++17 | Modern features, libfreenect compatibility | 2025-04 | C++20 (too new for some toolchains) |
| Metal for graphics | macOS native, better performance | 2025-04 | OpenGL (deprecated on macOS), Vulkan (via MoltenVK) |
| Error codes over exceptions | Hardware code tradition, cleaner recovery | 2025-04 | Exceptions |
| GoogleTest bundled | Consistent test environment | 2025-04 | System GoogleTest |

## Dependencies

```
┌─────────────────────────────────────────────────────┐
│                    Kinect XR                        │
├─────────────────────────────────────────────────────┤
│ Build Dependencies                                  │
│  ├── CMake 3.20+                                   │
│  ├── C++17 compiler (clang/gcc)                    │
│  └── GoogleTest 1.13.0 (bundled)                   │
├─────────────────────────────────────────────────────┤
│ Runtime Dependencies                                │
│  ├── libfreenect 0.7.x (user-provided)             │
│  └── OpenXR SDK 1.0.x (Phase 2+)                   │
├─────────────────────────────────────────────────────┤
│ Platform Dependencies                               │
│  ├── macOS 10.15+ (primary)                        │
│  └── Metal framework (Phase 2+)                    │
└─────────────────────────────────────────────────────┘
```

## Testing Strategy

### Test Pyramid

```
         ┌─────────────┐
         │  E2E Tests  │ ← Hardware required, minimal
         │  (Future)   │
         └─────────────┘
        ┌───────────────┐
        │  Integration  │ ← Hardware required, skip when unavailable
        │    Tests      │
        └───────────────┘
       ┌─────────────────┐
       │   Unit Tests    │ ← No hardware, run everywhere
       │  (Primary)      │
       └─────────────────┘
```

### Test Categories

| Category | Location | Hardware | Purpose |
|----------|----------|----------|---------|
| Unit | `tests/unit/` | No | Logic validation, error handling |
| Integration | `tests/integration/` | Yes | Hardware interaction, streams |
| E2E | Future | Yes | Full OpenXR workflow |

### Mocking Strategy

Current: Integration tests skip when hardware unavailable.

Future (Phase 2+): Consider libfreenect mock for CI:
```cpp
class MockFreenectContext {
  // Simulate device responses for deterministic testing
};
```

## Build System

### CMake Targets

```cmake
# Libraries
kinect_xr_device      # Device abstraction layer
kinect_xr_runtime     # OpenXR runtime (Phase 2+)

# Executables
kinect_xr_runtime     # Main test/demo executable
unit_tests            # Unit test executable
integration_tests     # Integration test executable
```

### Directory Structure

```
kinect-xr/
├── CMakeLists.txt           # Root build configuration
├── CLAUDE.md                # AI assistant instructions
├── docs/
│   ├── PRD.md              # Product requirements
│   ├── ARCHITECTURE.md     # This document
│   └── planning/           # Research and implementation guides
├── include/kinect_xr/
│   └── device.h            # Public headers
├── rules/
│   ├── TESTING.md          # Test conventions
│   └── CONVENTIONS.md      # Code style
├── specs/
│   └── archive/            # Completed specs
├── src/
│   ├── device.cpp          # Device implementation
│   └── kinect_xr_runtime.cpp
├── tests/
│   ├── unit/               # Hardware-independent
│   └── integration/        # Hardware-dependent
└── tmp/                    # Ephemeral (gitignored)
```

## Performance Considerations

### Data Throughput

| Stream | Resolution | Depth | Rate | Bandwidth |
|--------|------------|-------|------|-----------|
| Depth | 640x480 | 11-bit | 30fps | ~5.6 MB/s |
| RGB | 640x480 | 24-bit | 30fps | ~27.6 MB/s |
| Combined | - | - | 30fps | ~33 MB/s |

### Optimization Strategies

1. **Zero-copy where possible** - Direct buffer access from libfreenect
2. **Frame dropping** - If processing can't keep up, drop oldest
3. **Parallel pipelines** - Depth and RGB processing in separate threads
4. **Metal compute shaders** - GPU-accelerated depth conversion (Phase 4)

## Security Considerations

- **USB access:** Requires appropriate permissions on macOS
- **No network:** Runtime is local-only (WebSocket bridge is separate application)
- **No authentication:** Personal use, not multi-user

## Future Extensibility

### Kinect 2 Support (Not Planned)

Would require:
- libfreenect2 integration
- Higher resolution handling (1920x1080 RGB, 512x424 depth)
- Different depth format (13-bit)
- New device abstraction (share interface, different implementation)

### Multi-Device Support

Current: Single device via `deviceId` in config.
Future: Device manager coordinating multiple KinectDevice instances.

### Linux/Windows Support

Architecture is platform-agnostic except:
- Metal dependency (swap for Vulkan/OpenGL)
- Runtime registration paths differ by OS

## Revision History

| Date | Version | Changes |
|------|---------|---------|
| 2025-04 | 0.1.0 | Initial architecture |
| 2026-02 | 0.2.0 | Formalized during compliance restructure |
