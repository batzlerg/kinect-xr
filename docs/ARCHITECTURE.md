# Kinect XR - Architecture Document

## Overview

C++ OpenXR runtime and WebSocket bridge layered on libfreenect for Kinect 1 hardware access. The architecture provides two parallel paths to Kinect depth data: an OpenXR runtime for native XR applications and a WebSocket bridge for browser-based applications.

## Dual-Path Strategy

Kinect XR provides **two sibling paths** to the same underlying KinectDevice:

```
                        ┌─────────────────────────────────┐
                        │         KinectDevice            │
                        │   (Device Abstraction Layer)    │
                        └─────────────┬───────────────────┘
                                      │
                    ┌─────────────────┴─────────────────┐
                    │                                   │
           ┌────────▼────────┐               ┌─────────▼─────────┐
           │  OpenXR Runtime │               │  WebSocket Bridge │
           │   (Phase 2+)    │               │    (Complete)     │
           └────────┬────────┘               └─────────┬─────────┘
                    │                                   │
           ┌────────▼────────┐               ┌─────────▼─────────┐
           │ Native XR Apps  │               │  Browser Apps     │
           │ Unity, Unreal   │               │  P5.js, Three.js  │
           └─────────────────┘               └───────────────────┘
```

### Why Two Paths?

**Chrome macOS WebXR Limitation:** Chrome's WebXR implementation is architecturally bound to Direct3D 11 on Windows. It requires:
- `XR_EXT_win32_appcontainer_compatible` extension
- D3D11 graphics backend
- Windows-specific security model

There is **no Metal binding** and no path to one. This is not a missing feature but an architectural decision in Chromium. macOS browsers will never discover local OpenXR runtimes for WebXR sessions.

### Decision Tree

```
Application Type?
├── Native XR (Unity/Unreal/custom)
│   └── Use OpenXR Runtime (Phase 2+)
│       - Standard OpenXR API
│       - Runtime registration via XR_RUNTIME_JSON
│       - Depth extension: XR_KHR_composition_layer_depth
│
└── Browser-based (P5.js/Three.js/web)
    └── Use WebSocket Bridge
        - Connect to ws://localhost:8765/kinect
        - Use KinectClient.js library
        - 30Hz RGB + depth streaming
```

### Bridge is Permanent, Not Temporary

The WebSocket bridge is a **first-class citizen**, not a workaround or stepping stone:

| Aspect | OpenXR Runtime | WebSocket Bridge |
|--------|----------------|------------------|
| Consumer | Native applications | Browser applications |
| Status | In development (Phase 2) | Complete |
| Purpose | XR ecosystem integration | Creative coding, web apps |
| Relationship | Sibling | Sibling |

Both paths consume the same KinectDevice layer. Neither depends on the other.

## System Context

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                             Applications                                     │
│                                                                              │
│    Native Path                              Browser Path                     │
│    ──────────                               ────────────                     │
│  ┌──────────────┐  ┌──────────────┐      ┌──────────────┐  ┌──────────────┐ │
│  │ Native OpenXR │  │ Unity/Unreal │      │   P5.js      │  │  Three.js    │ │
│  │ Applications  │  │ (OpenXR)     │      │   Sketches   │  │  WebXR Apps  │ │
│  └──────┬───────┘  └──────┬───────┘      └──────┬───────┘  └──────┬───────┘ │
└─────────┼─────────────────┼─────────────────────┼─────────────────┼─────────┘
          │                 │                     │                 │
          └────────┬────────┘                     └────────┬────────┘
                   │                                       │
                   ▼                                       ▼
┌─────────────────────────────────┐      ┌─────────────────────────────────────┐
│      OpenXR Runtime (Phase 2+)  │      │     WebSocket Bridge (COMPLETE)     │
│  ┌───────────────────────────┐  │      │  ┌─────────────────────────────────┐│
│  │  OpenXR API Layer         │  │      │  │  WebSocket Server               ││
│  │  - Instance/Session mgmt  │  │      │  │  - ws://localhost:8765/kinect   ││
│  │  - Swapchain handling     │  │      │  │  - JSON control protocol        ││
│  │  - Depth extension        │  │      │  │  - Binary frame streaming       ││
│  └───────────────────────────┘  │      │  └─────────────────────────────────┘│
│  ┌───────────────────────────┐  │      │  ┌─────────────────────────────────┐│
│  │  Data Transformation      │  │      │  │  KinectClient.js (browser)      ││
│  │  - Depth format conv.     │  │      │  │  - Frame parsing                ││
│  │  - Coordinate mapping     │  │      │  │  - Stream subscription          ││
│  └───────────────────────────┘  │      │  └─────────────────────────────────┘│
└────────────────┬────────────────┘      └──────────────────┬──────────────────┘
                 │                                          │
                 │      BOTH PATHS CONSUME SAME DEVICE      │
                 │                                          │
                 └─────────────────┬────────────────────────┘
                                   ▼
┌─────────────────────────────────────────────────────────────────┐
│          Device Abstraction Layer (Phase 1 - COMPLETE)          │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  KinectDevice class                                        │ │
│  │  - Initialization and cleanup (RAII)                       │ │
│  │  - Stream management (start/stop, callbacks)               │ │
│  │  - Motor control (tilt, LED, status)                       │ │
│  │  - Configuration handling                                  │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────┐
│                         libfreenect                             │
│  - USB isochronous transfer                                     │
│  - Raw depth/RGB frame capture                                  │
│  - Motor/LED control                                            │
└─────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────┐
│                       Kinect Hardware                           │
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

  // Stream management
  DeviceError startStreams();  // Start depth and RGB streaming
  DeviceError stopStreams();   // Stop streaming and clean up
  bool isStreaming() const;    // Query streaming state

  // Motor control (Phase 6)
  DeviceError setTiltAngle(double degrees);  // Set tilt (-27 to +27)
  DeviceError getTiltAngle(double& outAngle); // Get current angle
  DeviceError setLED(LEDState state);        // Set LED state
  DeviceError getMotorStatus(MotorStatus& outStatus); // Get full status

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
- Callback-based streaming using libfreenect C API patterns
- Static C callbacks bridge to KinectDevice instance methods
- Thread safety deferred to Phase 4 (single-threaded usage for Phase 1)

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

### Current (Phase 1 - Complete)

```
Kinect USB ─► libfreenect ─► C callbacks ─► KinectDevice ─► Test Application
                 │                              │
                 │                              └── Streaming state: start/stop/isStreaming
                 │
                 └── Isochronous USB (30fps depth, 30fps RGB)
                      ├── freenect_start_depth() / freenect_stop_depth()
                      └── freenect_start_video() / freenect_stop_video()

Notes:
- Callbacks registered via freenect_set_depth_callback() / freenect_set_video_callback()
- Static C callbacks use freenect_get_user() to retrieve KinectDevice* instance
- Frame data flows through callbacks (raw void* buffers + timestamps)
- Streaming state explicitly tracked to prevent start/stop errors
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
| Unit | `tests/unit/` | Conditional | Logic validation, error handling, stream state |
| Integration | `tests/integration/` | Conditional | Hardware interaction, OpenXR runtime discovery |
| Runtime | `tests/runtime/` | No | OpenXR API compliance (no device required) |

**Hardware-Optional Testing:**
- Tests use `GTEST_SKIP()` when Kinect not detected
- Unit tests validate stream lifecycle without actual frame capture
- Hardware-dependent tests run only when device connected
- Script `scripts/run-hardware-tests.sh` handles sudo elevation

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

## WebSocket Bridge Architecture (Complete)

The WebSocket bridge provides browser access to Kinect depth/RGB streams. This is the **only viable path** for browser-based applications on macOS.

```
┌─────────────────────────────────────────────────────────────────┐
│                    Browser (Chrome/Firefox/Safari)              │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  P5.js / Three.js Application                            │   │
│  │  - depth-field example (particle visualization)          │   │
│  │  - silhouette example (depth masking)                    │   │
│  └────────────────────────┬─────────────────────────────────┘   │
│                           │                                     │
│  ┌────────────────────────▼─────────────────────────────────┐   │
│  │  KinectClient.js                                         │   │
│  │  - WebSocket connection management                       │   │
│  │  - Frame parsing (RGB888, uint16 depth)                  │   │
│  │  - Subscription control                                  │   │
│  └────────────────────────┬─────────────────────────────────┘   │
└───────────────────────────┼─────────────────────────────────────┘
                            │ ws://localhost:8765/kinect
┌───────────────────────────▼─────────────────────────────────────┐
│  WebSocket Bridge Server (C++)                                  │
│  - JSON control messages (hello, subscribe, status, error)      │
│  - Binary frames: 8-byte header + pixel data                    │
│  - RGB: 640x480 RGB888 @ 30Hz (27.6 MB/s)                       │
│  - Depth: 640x480 uint16 @ 30Hz (18.4 MB/s)                     │
│  - Mock mode: --mock flag for development without hardware      │
└───────────────────────────┬─────────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────────┐
│  KinectDevice (Phase 1 - Complete)                              │
│  - 30 Hz RGB + Depth streaming                                  │
│  - Thread-safe frame cache                                      │
└─────────────────────────────────────────────────────────────────┘
```

### WebSocket Protocol

The bridge uses a simple protocol (see `specs/archive/008-WebSocketBridgeProtocol.md`):

1. **Connection:** Client connects to `ws://localhost:8765/kinect`
2. **Hello:** Server sends capabilities (resolution, frame rate, formats)
3. **Subscribe:** Client requests streams (`rgb`, `depth`, or both)
4. **Frames:** Server sends binary frames with 8-byte header (frame ID + stream type)
5. **Status:** Periodic updates on connection state and dropped frames

### Chrome macOS WebXR Limitation (Architectural)

Chrome's WebXR implementation is **architecturally bound to Direct3D 11**:

| Requirement | Chrome's Expectation | macOS Reality |
|-------------|---------------------|---------------|
| Graphics API | D3D11 | Metal only |
| Extension | `XR_EXT_win32_appcontainer_compatible` | Not applicable |
| Security model | Windows AppContainer | macOS sandboxing |
| OpenXR discovery | Windows registry | XR_RUNTIME_JSON |

This is not a missing feature or bug. Chromium's WebXR backend is designed around Windows security and graphics models. There is no Metal binding and no path to one.

**Implication:** The WebSocket bridge is the permanent solution for browser-based Kinect XR on macOS, not a workaround waiting to be replaced.

### Motor Control (Phase 6)

The WebSocket bridge also exposes Kinect motor control (tilt, LED, accelerometer) via the same WebSocket connection, enabling PTZ-style sensor positioning for browser-based applications.

```
┌─────────────────────────────────────────────────────────────────┐
│                    Browser Application                          │
│  - Send motor.setTilt { angle: 15 }                             │
│  - Receive motor.status { angle: 15.2, status: "STOPPED" }      │
└───────────────────────────┬─────────────────────────────────────┘
                            │ ws://localhost:8765/kinect
┌───────────────────────────▼─────────────────────────────────────┐
│  WebSocket Bridge Server                                        │
│  - Route motor commands: motor.setTilt, motor.setLed, etc.      │
│  - Rate limiting: 500ms minimum interval                        │
│  - Status polling: 150ms during movement                        │
└───────────────────────────┬─────────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────────┐
│  KinectDevice Motor Methods                                     │
│  - setTiltAngle(degrees) → clamp to [-27, 27]                   │
│  - getTiltAngle() → current angle                               │
│  - setLED(state) → 6 states (off, green, red, yellow, blink)    │
│  - getMotorStatus() → angle, status, accelerometer              │
└───────────────────────────┬─────────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────────┐
│  libfreenect Motor API                                          │
│  - freenect_set_tilt_degs() → blocking position control         │
│  - freenect_get_tilt_state() → STOPPED/MOVING/LIMIT             │
│  - freenect_set_led() → LED state control                       │
│  - freenect_get_mks_accel() → gravity-corrected accelerometer   │
└─────────────────────────────────────────────────────────────────┘
```

#### Motor Control Data Flow

```
1. Client sends command:
   {"type": "motor.setTilt", "angle": 15}

2. Bridge server receives:
   - Checks rate limit (500ms since last command)
   - If violated: sends motor.error {"code": "RATE_LIMITED"}
   - Clamps angle to [-27, 27]

3. Device layer executes:
   - kinectDevice->setTiltAngle(15)
   - Wraps freenect_set_tilt_degs(dev, 15)
   - Returns DeviceError status

4. Status query:
   - kinectDevice->getMotorStatus(status)
   - Returns: angle, TiltStatus, accel X/Y/Z

5. Bridge server responds:
   - Sends motor.status to requesting client
   - Starts status polling loop if motor is MOVING

6. Status polling (during movement):
   - Poll every 150ms in broadcastLoop()
   - Broadcast motor.status to all clients
   - Stop when motor reaches STOPPED or LIMIT state
```

#### Protocol Messages

**Commands (client → server):**
```json
// Set tilt angle (-27 to +27 degrees)
{"type": "motor.setTilt", "angle": 15}

// Set LED state
{"type": "motor.setLed", "state": "green"}

// Reset to level position (0 degrees)
{"type": "motor.reset"}

// Query current status
{"type": "motor.getStatus"}
```

**Events (server → client):**
```json
// Status update (sent after commands and during movement)
{
  "type": "motor.status",
  "angle": 15.2,
  "status": "STOPPED",  // or "MOVING", "LIMIT"
  "accelerometer": {"x": 0.1, "y": 0.2, "z": 9.8}
}

// Error response
{
  "type": "motor.error",
  "code": "RATE_LIMITED",
  "message": "Minimum 500ms between tilt commands"
}
```

#### Rate Limiting

Motor commands are rate-limited to prevent hardware damage:

- **Tilt commands** (`motor.setTilt`, `motor.reset`): 500ms minimum interval
- **LED commands** (`motor.setLed`): No rate limit (stateless)
- **Status queries** (`motor.getStatus`): No rate limit (read-only)

Rate limit violations return `motor.error` with code `RATE_LIMITED` (rejected, not queued).

#### Status Polling

When a tilt command triggers motor movement:

1. Bridge server sets `motorMoving_ = true`
2. `broadcastLoop()` polls `getMotorStatus()` every 150ms
3. Broadcasts `motor.status` events to all connected clients
4. Polling stops when `status == STOPPED` or `status == LIMIT`

Status polling events omit accelerometer data to reduce message size. Clients can query full status via `motor.getStatus` command.

#### Hardware Constraints

- **Position-based control only:** Kinect motors use absolute positioning, not continuous PTZ speed control
- **Blocking libfreenect calls:** Motor commands block until hardware responds (typically <100ms)
- **Angle range:** Hardware supports -27° to +27° (±31° physical limit, software clamped to safe range)
- **Firmware loading:** Models 1473/1517 require runtime firmware upload (not needed for Model 1414)

## Security Considerations

- **USB access:** Requires appropriate permissions on macOS
- **WebSocket bridge:** localhost-only, no authentication (personal use)
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
| 2026-02-05 | 0.3.0 | Added Dual-Path Strategy section; documented Chrome macOS WebXR limitation; updated system context to show bridge and runtime as siblings |
| 2026-02-06 | 0.4.0 | Added Motor Control section (Phase 6); documented WebSocket motor protocol, rate limiting, status polling |
