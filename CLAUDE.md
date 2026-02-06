# Kinect XR

C++ OpenXR runtime for Kinect 1 hardware, enabling legacy Kinect devices for modern AR/VR applications with a path to WebXR integration.

## Project Status

**Current Phase:** Phase 1 (Device Integration) - ~75% complete
**Next Milestone:** Stream management implementation

## Build

**Prerequisites:**
- CMake 3.20+
- C++17 compiler
- libfreenect (install via Homebrew: `brew install libfreenect`)

```bash
cmake -B build -S .
cmake --build build
```

## Test

```bash
./test.sh
```

**Note:** Integration tests require physical Kinect hardware. They auto-skip when no device is connected.

## Architecture

```
src/
├── kinect_xr_runtime.cpp  # Main executable (test harness)
└── device.cpp             # KinectDevice implementation

include/kinect_xr/
└── device.h               # Device abstraction interface

tests/
├── unit/                  # Hardware-independent tests
└── integration/           # Requires Kinect hardware
```

## Development Phases

| Phase | Description | Status |
|-------|-------------|--------|
| 1. Device Integration | libfreenect abstraction, stream management | In Progress |
| 2. OpenXR Runtime | Core OpenXR API implementation | Not Started |
| 3. Depth Extensions | XR_KHR_composition_layer_depth | Not Started |
| 4. Data Pipeline | Depth format conversion, coordinate mapping | Not Started |
| 5. Registration | macOS runtime registration | Not Started |
| 6. Motor Control | Optional vendor extension | Deferred |

## Key Patterns

- **RAII:** All resources managed via constructors/destructors
- **Error handling:** Return `DeviceError` enum (not exceptions)
- **Testing:** Unit tests run without hardware; integration tests use `GTEST_SKIP()`

## Dependencies

| Dependency | Strategy | Notes |
|------------|----------|-------|
| libfreenect | User-provided | `brew install libfreenect` |
| GoogleTest | Bundled (FetchContent) | v1.13.0 |
| OpenXR | Not yet integrated | Phase 2 |

## OpenXR Runtime Development

To use the Kinect XR runtime during development:

```bash
source scripts/register-runtime.sh
```

This sets the `XR_RUNTIME_JSON` environment variable to point to our manifest. Any OpenXR application will then use the Kinect XR runtime.

### Testing the Runtime

**Internal Tests:**
```bash
./build/bin/runtime_tests
```

**External Validation:**

The runtime can be tested with OpenXR SDK tools:

1. **hello_xr** (bundled with OpenXR SDK):
   ```bash
   source scripts/register-runtime.sh
   ./build/_deps/openxr-build/src/tests/hello_xr/hello_xr -g Vulkan
   ```

2. **OpenXR Explorer** (if installed):
   ```bash
   source scripts/register-runtime.sh
   openxr_explorer
   ```
   The Kinect XR runtime should appear in the runtime list with:
   - Name: "Kinect XR Runtime"
   - Extension: XR_KHR_composition_layer_depth

3. **Manual Verification:**
   ```bash
   source scripts/register-runtime.sh
   echo $XR_RUNTIME_JSON  # Should show path to manifest
   ```

## Testing Without Hardware

All unit tests pass without physical Kinect. Integration tests will skip gracefully:
```
[  SKIPPED ] No Kinect device connected, skipping test
```

Runtime tests (M6) do not require Kinect hardware and test OpenXR API compliance.

## Docs

- `docs/PRD.md` - Product requirements
- `docs/ARCHITECTURE.md` - System design
- `docs/planning/` - Implementation guides and research
